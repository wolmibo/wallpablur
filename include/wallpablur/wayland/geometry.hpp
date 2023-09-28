#ifndef WALLPABLUR_WAYLAND_GEOMETRY_HPP_INCLUDED
#define WALLPABLUR_WAYLAND_GEOMETRY_HPP_INCLUDED

#include <cmath>
#include <cstdint>

namespace wayland {

class geometry {
  public:
    bool operator==(const geometry&) const = default;

    [[nodiscard]] bool same_pixel_size(const geometry& rhs) const {
      return pixel_width() == rhs.pixel_width() && pixel_height() == rhs.pixel_height();
    }



    [[nodiscard]] bool empty() const {
      return width_ * height_ == 0 || std::abs(scale_) < 1e-3;
    }



    [[nodiscard]] uint32_t pixel_width()    const { return width_;  }
    [[nodiscard]] uint32_t pixel_height()   const { return height_; }

    [[nodiscard]] uint32_t logical_width()  const { return width_ / scale_;  }
    [[nodiscard]] uint32_t logical_height() const { return height_ / scale_; }

    [[nodiscard]] float    scale()          const { return scale_;  }



    void pixel_width (uint32_t width)  { width_  = width;  }
    void pixel_height(uint32_t height) { height_ = height; }

    void scale       (float s)         { scale_  = s;      }



  private:
    uint32_t width_ {0};
    uint32_t height_{0};
    float    scale_ {1};
};

}

#endif // WALLPABLUR_WAYLAND_GEOMETRY_HPP_INCLUDED
