#include "wallpablur/wayland/client.hpp"

#include "wallpablur/exception.hpp"
#include "wallpablur/wayland/output.hpp"

#include <cstdint>
#include <cstdio>
#include <string>
#include <string_view>

#include <logcerr/log.hpp>



namespace {
  [[nodiscard]] std::string format_message(const char* fmt, va_list argp) {
    if (auto char_count = vsnprintf(nullptr, 0, fmt, argp); char_count >= 0) {
      std::string output;
      output.resize(char_count);
      vsnprintf(output.data(), output.size() + 1, fmt, argp);
      return output;
    }

    return "<error formatting message>";
  }



  [[noreturn]] void wl_log_cb(const char* fmt, va_list argp) {
    throw exception{format_message(fmt, argp), true, {}}; 
  }



  [[nodiscard]] wl_ptr<wl_display> connect_to_wayland_display() {
    wl_log_set_handler_client(wl_log_cb);

    wl_ptr<wl_display> display{wl_display_connect(nullptr)};

    if (!display) {
      throw exception{"unable to connect to wayland compositor"};
    }

    return display;
  }



  template<typename T>
  void require_interface(
      const wl_ptr<T>&     interface,
      std::source_location location = std::source_location::current()
  ) {
    if (!interface) {
      throw exception{
        std::format("required wayland interface \"{}\" missing", wayland_interface<T>::get()->name),
        false,
        location
      };
    }
  }



  template<typename Fnc, typename Str>
  void check_errno(
      Str&& message,
      Fnc&& fnc,
      std::source_location location = std::source_location::current()
  ) {
    errno = 0;

    if (std::invoke(std::forward<Fnc>(fnc))) {
      return;
    }

    std::string msg{};

    if (auto error = std::make_error_code(static_cast<std::errc>(errno))) {
      msg = std::format("{}: {} ({})", std::forward<Str>(message), error.message(), error.value());
    } else {
      msg = std::format("{}: unknown error", std::forward<Str>(message));
    }

    throw exception(msg, true, location);
  }
}



wayland::client::client() :
  display_ {connect_to_wayland_display()},
  context_{std::make_shared<egl::context>(display_.get())},
  registry_{wl_display_get_registry(display_.get())}
{
  wl_registry_add_listener(registry_.get(), &registry_listener_, this);
}



void wayland::client::dispatch() {
  check_errno("wayland: unable to dispatch", [&] {
      return wl_display_dispatch(display_.get()) >= 0;
  });
}


void wayland::client::roundtrip() {
  check_errno("wayland: unable to perform roundtrip", [&] {
      return wl_display_roundtrip(display_.get()) >= 0;
  });
}



void wayland::client::explore() {
  dispatch();

  require_interface(compositor_);
  require_interface(layer_shell_);
  require_interface(viewporter_);

  roundtrip();
}





namespace {
  template<typename T>
  [[nodiscard]] wl_ptr<T> registry_bind(
      wl_registry* registry,
      uint32_t     name,
      uint32_t     version
  ) {
    return wl_ptr<T>{static_cast<T*>(
      wl_registry_bind(registry, name, wayland_interface<T>::get(), version)
    )};
  }


  [[nodiscard]] bool is_interface(std::string_view value, const wl_interface& iface) {
    return value == iface.name;
  }
}



void wayland::client::registry_global_(
    void*        data,
    wl_registry* registry,
    uint32_t     name,
    const char*  interface,
    uint32_t     /*version*/
) {
  auto* self = static_cast<client*>(data);

  if (is_interface(interface, wl_compositor_interface)) {
    self->compositor_ = registry_bind<wl_compositor>(registry, name, 4);
  } else if (is_interface(interface, wl_output_interface)) {
    if (self->output_add_callback_) {
      self->output_add_callback_(name,
        std::make_unique<output>(registry_bind<wl_output>(registry, name, 4), *self));
    }
  } else if (is_interface(interface, zwlr_layer_shell_v1_interface)) {
    self->layer_shell_ = registry_bind<zwlr_layer_shell_v1>(registry, name, 1);
  } else if (is_interface(interface, wp_viewporter_interface)) {
    self->viewporter_ = registry_bind<wp_viewporter>(registry, name, 1);
  }
}



void wayland::client::registry_global_remove_(
    void*        data,
    wl_registry* /*registry*/,
    uint32_t     name
) {
  auto* self = static_cast<client*>(data);

  if (self->output_remove_callback_) {
    self->output_remove_callback_(name);
  }
}
