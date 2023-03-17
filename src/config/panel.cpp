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



rectangle config::panel::to_rect(float w, float h) const {
  auto width  = extents(size.width, anchor.left() && anchor.right(),
      margin.left + margin.right, w);

  auto height = extents(size.height, anchor.top() && anchor.bottom(),
      margin.top + margin.bottom, h);

  return {
    offset(margin.left, anchor.left(), margin.right,  anchor.right(),  width,  w),
    offset(margin.top,  anchor.top(),  margin.bottom, anchor.bottom(), height, h),
    width,
    height
  };
}
