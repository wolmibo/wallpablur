#include "wallpablur/workspace.hpp"

#include <limits>



namespace {
  [[nodiscard]] rectangle bounding_box(std::span<const surface> surfaces) {
    vec2<float> min{std::numeric_limits<float>::max()};
    vec2<float> max{std::numeric_limits<float>::min()};

    for (const auto& surface: surfaces) {
      min = ::min(min, surface.rect().min());
      max = ::max(max, surface.rect().max());
    }

    return {min, max - min};
  }


  [[nodiscard]] bool left_below_of(const vec2<float>& lhs, const vec2<float>& rhs) {
    return lhs.x() <= rhs.x() && lhs.y() <= rhs.y();
  }


  [[nodiscard]] bool covers(std::span<const surface> surfaces, vec2<float> area) {
    auto bbox = bounding_box(surfaces);

    return left_below_of(bbox.pos(), vec2{0.1f}) &&
      left_below_of(area - vec2{0.1f}, bbox.size());
  }
}



bool workspace::test_flag(workspace_flag flag) const {
  if (!::test_flag(flags_set_, flag)) {
    switch (flag) {
      case workspace_flag::covered:
        set_flag(flags_, flag, covers(surfaces(), size_));
        break;

      case workspace_flag::eoec_marker:
        break;
    }

    set_flag(flags_set_, flag);
  }

  return ::test_flag(flags_, flag);
}
