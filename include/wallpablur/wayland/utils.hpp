#ifndef WALLPABLUR_WAYLAND_UTILS_HPP_INCLUDED
#define WALLPABLUR_WAYLAND_UTILS_HPP_INCLUDED

#include <memory>

#include <wayland-client.h>



template<typename T> struct wayland_deleter{};
template<typename T> struct wayland_interface{};

// NOLINTBEGIN(*macro-usage,*macro-parentheses)
#define DEFINE_WAYLAND_DELETER(x, y)           \
  class x;                                     \
  template<> struct wayland_deleter<x> {       \
    void operator()(x* ptr) const {            \
      x ##_ ## y (ptr);                        \
    }                                          \
  };


#define DEFINE_WAYLAND_INTERFACE(x)            \
  class x;                                     \
  template<> struct wayland_interface<x> {     \
    static const wl_interface* get() {         \
      return & x ##_interface;                 \
    }                                          \
  };                                           \
// NOLINTEND(*macro-usage,*macro-parentheses)


DEFINE_WAYLAND_DELETER(wl_display,  disconnect);
DEFINE_WAYLAND_DELETER(wl_registry, destroy);

DEFINE_WAYLAND_DELETER  (wl_surface, destroy);

DEFINE_WAYLAND_DELETER  (wl_compositor, destroy);
DEFINE_WAYLAND_INTERFACE(wl_compositor);

DEFINE_WAYLAND_DELETER  (wl_output, destroy);
DEFINE_WAYLAND_INTERFACE(wl_output);


DEFINE_WAYLAND_DELETER(wl_callback, destroy);

DEFINE_WAYLAND_DELETER(wl_region, destroy);

#ifdef WAYLAND_EGL_H
DEFINE_WAYLAND_DELETER(wl_egl_window, destroy);
#endif


#ifdef ZWLR_LAYER_SURFACE_V1_INTERFACE
DEFINE_WAYLAND_DELETER  (zwlr_layer_surface_v1, destroy);
DEFINE_WAYLAND_DELETER  (zwlr_layer_shell_v1,   destroy);
DEFINE_WAYLAND_INTERFACE(zwlr_layer_shell_v1);
#endif


#ifdef WP_VIEWPORTER_INTERFACE
DEFINE_WAYLAND_DELETER  (wp_viewport,   destroy);
DEFINE_WAYLAND_DELETER  (wp_viewporter, destroy);
DEFINE_WAYLAND_INTERFACE(wp_viewporter);
#endif


#undef DEFINE_WAYLAND_INTERFACE
#undef DEFINE_WAYLAND_DELETER



template<typename T>
using wl_ptr = std::unique_ptr<T, wayland_deleter<T>>;

#endif // WALLPABLUR_WAYLAND_UTILS_HPP_INCLUDED
