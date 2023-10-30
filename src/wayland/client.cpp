#include "wallpablur/wayland/client.hpp"

#include "wallpablur/wayland/output.hpp"

#include <array>
#include <cstdint>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <string_view>

#include <logcerr/log.hpp>



namespace {
  [[noreturn]] void wl_log_cb(const char* fmt, va_list argp) {
    std::array<char, 1024> buffer{};
    vsnprintf(buffer.data(), buffer.size(), fmt, argp);
    throw std::runtime_error{std::string{"wayland protocol error: "} + buffer.data()};
  }

  [[nodiscard]] wl_ptr<wl_display> connect_to_wayland_display() {
    wl_log_set_handler_client(wl_log_cb);

    wl_ptr<wl_display> display{wl_display_connect(nullptr)};

    if (!display) {
      throw std::runtime_error{"unable to connect to wayland compositor"};
    }

    return display;
  }
}



wayland::client::client() :
  display_ {connect_to_wayland_display()},
  context_{std::make_shared<egl::context>(display_.get())},
  registry_{wl_display_get_registry(display_.get())}
{
  wl_registry_add_listener(registry_.get(), &registry_listener_, this);
}



void wayland::client::explore() {
  if (dispatch() == -1) {
    throw std::runtime_error{"wayland: unable to dispatch"};
  };

  if (!compositor_) {
    throw std::runtime_error{"wayland: unable to obtain interface wl_compositor"};
  }

  if (!layer_shell_) {
    throw std::runtime_error{"wayland: unable to obtain interface zwlr_layer_shell_v1"};
  }

  if (!viewporter_) {
    throw std::runtime_error{"wayland: unable to obtain interface wp_viewporter"};
  }

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
