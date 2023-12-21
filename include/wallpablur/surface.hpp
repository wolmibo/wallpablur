#ifndef WALLPABLUR_SURFACE_HPP_INCLUDED
#define WALLPABLUR_SURFACE_HPP_INCLUDED

#include "wallpablur/rectangle.hpp"

#include <string>
#include <string_view>
#include <utility>

#include <flags.hpp>





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



class surface {
  public:
    surface(
        rectangle               rect,
        std::string             app_id,
        flag_mask<surface_flag> mask = {},
        float                   radius      = 0.f
    ) :
      rect_  {rect},
      radius_{radius},

      app_id_{std::move(app_id)},
      mask_  {mask}
    {}



    bool operator==(const surface&) const = default;

    [[nodiscard]] rectangle&         rect()              { return rect_; }
    [[nodiscard]] const rectangle&   rect()        const { return rect_; }

    [[nodiscard]] float              radius()      const { return radius_; }



    [[nodiscard]] std::string_view               app_id() const { return app_id_;  }
    [[nodiscard]] const flag_mask<surface_flag>& flags()  const { return mask_; }



  private:
    rectangle               rect_;
    float                   radius_;

    std::string             app_id_;

    flag_mask<surface_flag> mask_;
};

#endif // WALLPABLUR_SURFACE_HPP_INCLUDED
