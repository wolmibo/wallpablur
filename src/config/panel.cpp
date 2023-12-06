#include "wallpablur/config/panel.hpp"



namespace {
  [[nodiscard]] constexpr float saturate_sub(float value, float sub) {
    if (value < sub) {
      return 0;
    }
    return value - sub;
  }



  [[nodiscard]] float extents(uint32_t extent, bool span, float margin, float screen) {
    if (extent == 0 && span) {
      return saturate_sub(screen, margin);
    }
    return extent;
  }



  [[nodiscard]] float offset(
      int32_t neg,
      bool    attach_neg,
      int32_t pos,
      bool    attach_pos,
      float   extent,
      float   screen
  ) {
    if (attach_neg && !attach_pos) {
      return neg;
    }

    if (!attach_neg && attach_pos) {
      return saturate_sub(screen, pos + extent);
    }

    return saturate_sub(screen / 2, (extent + neg + pos) / 2) + neg;
  }
}



rectangle config::panel::to_rect(const vec2<float>& screen_size) const {
  vec2 realized_size{
    extents(size.x(), anchor.left() && anchor.right(),
        margin.left + margin.right, screen_size.x()),

    extents(size.y(), anchor.top() && anchor.bottom(),
        margin.top + margin.bottom, screen_size.y())
  };

  return {
    {offset(margin.left, anchor.left(), margin.right,  anchor.right(),
        realized_size.x(), screen_size.x()),
     offset(margin.top,  anchor.top(),  margin.bottom, anchor.bottom(),
        realized_size.y(), screen_size.y())},
    realized_size
  };
}
