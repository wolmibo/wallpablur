#ifndef WALLPABLUR_SURFACE_HPP_INCLUDED
#define WALLPABLUR_SURFACE_HPP_INCLUDED

#include "wallpablur/rectangle.hpp"

#include <bitset>
#include <string>
#include <string_view>
#include <utility>





enum class surface_flag : size_t {
  panel,
  floating,
  tiled,
  decoration,

  focused,
  urgent,

  fullscreen,

  splitv,
  splith,
  tabbed,
  stacked,

  eoec_marker
};

using surface_flag_mask = std::bitset<std::to_underlying(surface_flag::eoec_marker) - 1>;

constexpr void set_surface_flag(surface_flag_mask& mask, surface_flag flag) {
  mask[std::to_underlying(flag)] = true;
}

[[nodiscard]] constexpr bool test_surface_flag(
    const surface_flag_mask& mask,
    surface_flag             flag
) {
  return mask[std::to_underlying(flag)];
}




class surface {
  public:
    surface(
        rectangle          rect,
        std::string        app_id,
        surface_flag_mask  mask        = {},
        float              radius      = 0.f
    ) :
      rect_       {rect},
      radius_     {radius},

      app_id_     {std::move(app_id)},
      mask_       {mask}
    {}



    bool operator==(const surface&) const = default;

    [[nodiscard]] rectangle&         rect()              { return rect_; }
    [[nodiscard]] const rectangle&   rect()        const { return rect_; }

    [[nodiscard]] float              radius()      const { return radius_; }

    [[nodiscard]] std::string_view   app_id()      const { return app_id_;  }

    [[nodiscard]] const surface_flag_mask& flags() const { return mask_; }



    [[nodiscard]] bool has_flag(surface_flag flag) const {
      return mask_[std::to_underlying(flag)];
    }



  private:
    rectangle          rect_;
    float              radius_;

    std::string        app_id_;

    surface_flag_mask  mask_;
};

#endif // WALLPABLUR_SURFACE_HPP_INCLUDED
