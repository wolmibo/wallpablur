#include "wallpablur/application.hpp"
#include "wallpablur/application-args.hpp"
#include "wallpablur/config/config.hpp"
#include "wallpablur/output.hpp"
#include "wallpablur/wayland/output.hpp"
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



  namespace global_state {
    application* app = nullptr;
  }
}



application::application(const application_args& args) :
  i3ipc_ {i3ipc_from_args_and_config(args)},
  texture_provider_{std::make_shared<::texture_provider>(wayland_client_.share_context())},

  app_start_{std::chrono::high_resolution_clock::now()}
{
  register_signal_handler();



  wayland_client_.set_output_add_cb([this](uint32_t name, auto op) {
    logcerr::verbose("added output {}", name);

    if (auto index = outputs_.find_index(name)) {
      logcerr::warn("output with id {} already existed", name);
      outputs_.value(*index) = std::make_unique<output>(std::move(op));
    } else {
      outputs_.emplace(name, std::make_unique<output>(std::move(op)));
    }
  });



  wayland_client_.set_output_remove_cb([this](uint32_t name) {
    if (auto index = outputs_.find_index(name)) {
      outputs_.erase(*index);
      logcerr::verbose("removed output {}", name);
    } else {
      logcerr::warn("cannot find output {} for removal", name);
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



application& app() {
  return *global_state::app;
}



std::shared_ptr<texture_provider> application::texture_provider() {
  return texture_provider_;
}



change_token<workspace> application::layout_token(std::string_view name) {
  if (i3ipc_) {
    return i3ipc_->layout_token(name);
  }

  return {};
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
  global_state::app = this;

  wayland_client_.explore();

  while (wayland_client_.dispatch() != -1 && !exit_signal_received) {}

  if (config::global_config().fade_out() == std::chrono::milliseconds{0}) {
    return EXIT_SUCCESS;
  }

  logcerr::log("stop signal received; send again to cancel fade out");

  register_signal_handler();
  exit_start_ = std::chrono::high_resolution_clock::now();

  while (wayland_client_.dispatch() != -1 && !exit_signal_received) {}

  global_state::app = nullptr;

  return EXIT_SUCCESS;
}
