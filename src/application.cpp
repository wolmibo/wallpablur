#include "wallpablur/application.hpp"
#include "wallpablur/application-args.hpp"
#include "wallpablur/config/config.hpp"
#include "wallpablur/wayland/surface.hpp"
#include "wallpablur/wm/i3ipc-socket.hpp"

#include <chrono>

#include <cstdlib>
#include <csignal>

#include <logcerr/log.hpp>



namespace {
  [[nodiscard]] std::optional<wm::i3ipc> i3ipc_from_args_and_config(
      const application_args& arg
  ) {
    if (config::global_config().disable_i3ipc()) {
      return {};
    }

    auto path = arg.socket_path.or_else([](){ return wm::find_i3_socket(); });
    if (!path) {
      logcerr::warn("Unable to find i3ipc socket\n"
        "Make sure --socket, SWAYSOCK, or I3SOCK is set correctly.\n"
        "Alternatively, use --disable-i3ipc to disable i3ipc.");
      return {};
    }

    try {
      return std::make_optional<wm::i3ipc>(*path);
    } catch (std::exception& ex) {
      logcerr::error(ex.what());
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
      default:
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
  i3ipc_ {i3ipc_from_args_and_config(args)},
  texture_provider_{std::make_shared<texture_provider>(wayland_client_.share_context())},

  app_start_{std::chrono::high_resolution_clock::now()}
{
  register_signal_handler();



  wayland_client_.set_output_add_cb([this](auto& output) {
    logcerr::verbose("added output {}", output.name());

    output_data data {
      .name          = std::string{output.name()},
      .painter       = layout_painter{
                         config::global_config().output_config_for(output.name()),
                         output.wallpaper_surface().share_context(),
                         output.has_clipping() ?
                            output.clipping_surface().share_context() : nullptr,
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
      logcerr::debug("removing output before it got a name");
      return;
    }

    auto name = output.name();

    logcerr::verbose("removed output {}", name);

    if (auto it = std::ranges::find_if(data_, [name](const auto& ptr){
          return ptr->name == name;
        }); it != data_.end()) {
      std::swap(*it, data_.back());
      data_.pop_back();
    } else {
      logcerr::warn("unable to remove output {} from list: not found", name);
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
  if (config::global_config().fade_in() > std::chrono::milliseconds{0}) {
    auto run = elapsed_since(app_start_);
    if (run < config::global_config().fade_in()) {
      return static_cast<float>(run.count())
        / static_cast<float>(config::global_config().fade_in().count())
        * config::global_config().opacity();
    }
  }
  if (!exit_start_) {
    return config::global_config().opacity();
  }

  auto elapsed = elapsed_since(*exit_start_);

  if (elapsed > config::global_config().fade_out()) {
    exit_signal_received = true;
    return 0.f;
  }

  return (1.f - (static_cast<float>(elapsed.count())
                 / static_cast<float>(config::global_config().fade_out().count())))
         * config::global_config().opacity();
}



int application::run() {
  wayland_client_.explore();

  while (wayland_client_.dispatch() != -1 && !exit_signal_received) {}

  if (config::global_config().fade_out() == std::chrono::milliseconds{0}) {
    return EXIT_SUCCESS;
  }

  logcerr::log("stop signal received; send again to cancel fade out");

  register_signal_handler();
  exit_start_ = std::chrono::high_resolution_clock::now();

  while (wayland_client_.dispatch() != -1 && !exit_signal_received) {}

  return EXIT_SUCCESS;
}
