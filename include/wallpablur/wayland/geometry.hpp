#ifndef WALLPABLUR_WAYLAND_GEOMETRY_HPP_INCLUDED
#define WALLPABLUR_WAYLAND_GEOMETRY_HPP_INCLUDED

#include "wallpablur/vec2.hpp"

#include <cmath>
#include <cstdint>

namespace wayland {



class geometry {
  public:
    bool operator==(const geometry&) const = default;

    [[nodiscard]] bool same_physical_size(const geometry& rhs) const {
      return physical_size() == rhs.physical_size();
    }



    [[nodiscard]] bool empty() const {
      return size_.x() * size_.y() == 0 || std::abs(scale_) < 1e-3;
    }


    [[nodiscard]] const vec2<uint32_t>& physical_size() const { return size_; }
    [[nodiscard]] vec2<float>           logical_size()  const {
      return floor(vec_cast<float>(size_) / scale_);
    }

    [[nodiscard]] float                 scale()         const { return scale_;  }



    void scale        (float s)                    { scale_  = s;  }
    void physical_size(const vec2<uint32_t>& size) { size_ = size; }



  private:
    vec2<uint32_t> size_ {0};
    float          scale_{1};
};

}

#endif // WALLPABLUR_WAYLAND_GEOMETRY_HPP_INCLUDED
