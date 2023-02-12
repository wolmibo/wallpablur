#include "wallpablur/application.hpp"
#include "wallpablur/application-args.hpp"
#include "wallpablur/config/config.hpp"
#include "wallpablur/wm/i3ipc-socket.hpp"

#include <chrono>
#include <fstream>

#include <cstdlib>
#include <csignal>

#include <logging/log.hpp>



namespace {
  [[nodiscard]] std::string read_file(const std::filesystem::path& path) {
      std::ifstream input(path, std::ios::ate);
      if (!input) {
        throw std::runtime_error{fmt::format("unable to read file at \"{}\"",
            path.string())};
      }

      auto size = input.tellg();
      std::string buffer;
      buffer.resize(size);

      input.seekg(0);

      input.read(buffer.data(), buffer.size());

      return buffer;
  }



  void try_set_working_directory(const std::filesystem::path& path) {
    std::error_code ec;
    std::filesystem::current_path(path, ec);

    if (ec) {
      logging::warn("unable to set working directory:\n\"{}\": {}",
          path.string(), ec.message());
    }
  }



  [[nodiscard]] config::config apply_args_config(
      config::config          cfg,
      const application_args& arg
  ) {
    if (arg.fade_in)   { cfg.fade_in(*arg.fade_in);     }
    if (arg.fade_out)  { cfg.fade_out(*arg.fade_out);   }
    if (arg.poll_rate) { cfg.poll_rate(*arg.poll_rate); }

    cfg.disable_i3ipc(arg.disable_i3ipc);

    return cfg;
  }



  [[nodiscard]] config::config config_from_args(const application_args& arg) {
    if (arg.image) {
      logging::verbose("using generic config");
      return config::config{arg.blur.to_config(*arg.image)};
    }

    if (arg.config_string) {
      logging::verbose("using config string from arguments");
      return config::config(*arg.config_string);
    }

    if (auto path = arg.config_path.or_else([](){ return config::find_config_file();})) {
      logging::verbose("using config located at \"{}\"", path->string());
      try_set_working_directory(std::filesystem::absolute(*path).parent_path());
      return config::config(read_file(*path));
    }

    logging::verbose("using default config");
    return config::config{};
  }



  [[nodiscard]] std::optional<wm::i3ipc> i3ipc_from_args_and_config(
      const application_args& arg,
      const config::config&   conf
  ) {
    if (conf.disable_i3ipc()) {
      return {};
    }

    auto path = arg.socket_path.or_else([](){ return wm::find_i3_socket(); });
    if (!path) {
      logging::warn("Unable to find i3ipc socket\n"
        "Make sure --socket, SWAYSOCK, or I3SOCK is set correctly.\n"
        "Alternatively, use --disable-i3ipc to disable i3ipc.");
      return {};
    }

    try {
      return std::make_optional<wm::i3ipc>(
          *path, arg.poll_rate.value_or(conf.poll_rate()));
    } catch (std::exception& ex) {
      logging::error(ex.what());
      return {};
    }
  }



  std::atomic<bool> exit_signal_received{false};

  void signal_handler(int signal) {
    switch (signal) {
      case SIGINT:
      case SIGTERM:
        exit_signal_received = true;
        break;
    }
  }

  void register_signal_handler() {
    exit_signal_received = false;

    std::signal(SIGINT,  signal_handler);
    std::signal(SIGTERM, signal_handler);
  }
}



application::application(const application_args& args) :
  config_{apply_args_config(config_from_args(args), args)},
  i3ipc_ {i3ipc_from_args_and_config(args, config_)},
  texture_provider_{std::make_shared<texture_provider>(wayland_client_.share_context())},

  app_start_{std::chrono::high_resolution_clock::now()}
{
  register_signal_handler();



  wayland_client_.set_output_add_cb([this](auto& output) {
    logging::verbose("added output {}", output.name());

    output_data data {
      .name          = std::string{output.name()},
      .painter       = layout_painter{
                         config_.output_config_for(output.name()),
                         output.share_context(),
                         texture_provider_
                       },
      .layout_source = {},
      .last_alpha    = alpha()
    };

    if (i3ipc_) {
      data.layout_source = i3ipc_->layout_token(output.name());
    }



    data_.push_back(std::make_unique<output_data>(std::move(data)));
    auto *data_ptr = data_.back().get();

    output.set_update_cb([data_ptr, this](auto geometry) {
      auto update = data_ptr->painter.update_geometry(geometry);
      update = update || data_ptr->layout_source.changed();

      if (std::fabs(data_ptr->last_alpha - alpha()) > 1.f/255.f) {
        update = true;
      }

      return update;
    });

    output.set_render_cb([data_ptr, this](auto geometry) {
      data_ptr->painter.update_geometry(geometry);
      data_ptr->last_alpha = alpha();
      data_ptr->painter.draw_layout(*data_ptr->layout_source.get(),
          data_ptr->last_alpha);
    });
  });



  wayland_client_.set_output_remove_cb([this](auto& output) {
    if (!output.ready()) {
      logging::debug("removing output before it got a name");
      return;
    }

    auto name = output.name();

    logging::verbose("removed output {}", name);

    if (auto it = std::ranges::find_if(data_, [name](const auto& ptr){
          return ptr->name == name;
        }); it != data_.end()) {
      std::swap(*it, data_.back());
      data_.pop_back();
    } else {
      logging::warn("unable to remove output {} from list: not found", name);
    }
  });

}




namespace {
  std::chrono::milliseconds elapsed_since(
      std::chrono::high_resolution_clock::time_point tp
  ) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::high_resolution_clock::now() - tp);
  }
}



float application::alpha() const {
  if (config_.fade_in() > std::chrono::milliseconds{0}) {
    auto run = elapsed_since(app_start_);
    if (run < config_.fade_in()) {
      return static_cast<float>(run.count()) /
        static_cast<float>(config_.fade_in().count());
    }
  }
  if (!exit_start_) {
    return 1.f;
  }

  auto elapsed = elapsed_since(*exit_start_);

  if (elapsed > config_.fade_out()) {
    exit_signal_received = true;
    return 0.f;
  }

  return 1.f - (static_cast<float>(elapsed.count())
                 / static_cast<float>(config_.fade_out().count()));
}



int application::run() {
  wayland_client_.explore();

  while (wayland_client_.dispatch() != -1 && !exit_signal_received) {}

  if (config_.fade_out() == std::chrono::milliseconds{0}) {
    return EXIT_SUCCESS;
  }

  logging::log("stop signal received; send again to cancel fade out");

  register_signal_handler();
  exit_start_ = std::chrono::high_resolution_clock::now();

  while (wayland_client_.dispatch() != -1 && !exit_signal_received) {}

  return EXIT_SUCCESS;
}
