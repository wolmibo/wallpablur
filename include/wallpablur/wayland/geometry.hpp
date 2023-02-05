#ifndef WALLPABLUR_WAYLAND_GEOMETRY_HPP_INCLUDED
#define WALLPABLUR_WAYLAND_GEOMETRY_HPP_INCLUDED

#include <cstdint>

namespace wayland {

struct geometry {
  uint32_t width;
  uint32_t height;
  uint32_t scale;



  bool operator==(const geometry&) const = default;

  [[nodiscard]] bool same_pixel_size(const geometry& rhs) const {
    return pixel_width() == rhs.pixel_width() && pixel_height() == rhs.pixel_height();
  }

  [[nodiscard]] uint32_t pixel_width()  const { return width  * scale; }
  [[nodiscard]] uint32_t pixel_height() const { return height * scale; }
};

}

#endif // WALLPABLUR_WAYLAND_GEOMETRY_HPP_INCLUDED
