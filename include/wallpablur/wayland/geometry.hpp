#ifndef WALLPABLUR_WAYLAND_GEOMETRY_HPP_INCLUDED
#define WALLPABLUR_WAYLAND_GEOMETRY_HPP_INCLUDED

#include <cstdint>

namespace wayland {

class geometry {
  public:
    bool operator==(const geometry&) const = default;

    [[nodiscard]] bool same_pixel_size(const geometry& rhs) const {
      return pixel_width() == rhs.pixel_width() && pixel_height() == rhs.pixel_height();
    }



    [[nodiscard]] bool empty() const { return width_ * height_ * scale_ == 0; }



    [[nodiscard]] uint32_t pixel_width()    const { return width_  * scale_; }
    [[nodiscard]] uint32_t pixel_height()   const { return height_ * scale_; }

    [[nodiscard]] uint32_t logical_width()  const { return width_;  }
    [[nodiscard]] uint32_t logical_height() const { return height_; }

    [[nodiscard]] uint32_t scale()          const { return scale_;  }



    void logical_width (uint32_t width)  { width_  = width;  }
    void logical_height(uint32_t height) { height_ = height; }

    void scale         (uint32_t s)      { scale_  = s;      }



  private:
    uint32_t width_ {0};
    uint32_t height_{0};
    uint32_t scale_ {1};
};

}

#endif // WALLPABLUR_WAYLAND_GEOMETRY_HPP_INCLUDED
