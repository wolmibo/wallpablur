#include "wallpablur/workspace.hpp"

#include <limits>



namespace {
  enum class totally_not_bool : bool { yes = true, no = false };



  void append_point(std::vector<float>& rows, std::vector<float>& cols, vec2<float> p) {
    cols.emplace_back(p.x());
    rows.emplace_back(p.y());
  }



  void sort_unique(std::vector<float>& list) {
    std::ranges::sort(list);
    auto ret = std::ranges::unique(list);
    list.erase(ret.begin(), ret.end());
  }



  [[nodiscard]] size_t find_index(std::span<const float> sorted_list, float value) {
    auto lb = std::ranges::lower_bound(sorted_list, value);

    if (lb == sorted_list.end()) {
      return sorted_list.size() - 1;
    }
    return lb - sorted_list.begin();
  }



  [[nodiscard]] std::vector<totally_not_bool> paint(
      std::span<const surface> surfaces,
      vec2<float>              area
  ) {
    std::vector<float> rows;
    std::vector<float> cols;

    append_point(rows, cols, vec2{0.f});
    append_point(rows, cols, area);

    for (const auto& surface: surfaces) {
      append_point(rows, cols, surface.rect().min());
      append_point(rows, cols, surface.rect().max());
    }

    sort_unique(rows);
    sort_unique(cols);

    std::vector<totally_not_bool> image(rows.size() * cols.size(), totally_not_bool::no);

    for (const auto& surface: surfaces) {
      auto min = surface.rect().min();
      auto max = surface.rect().max();

      auto max_x = find_index(cols, max.x());
      auto max_y = find_index(rows, max.y());

      for (size_t y = find_index(rows, min.y()); y <= max_y; ++y) {
        for (size_t x = find_index(cols, min.x()); x <= max_x; ++x) {
          image[y * cols.size() + x] = totally_not_bool::yes;
        }
      }
    }

    return image;
  }





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

    if (!left_below_of(bbox.pos(), vec2{0.1f}) ||
        !left_below_of(area - vec2{0.1f}, bbox.size())) {
      return false;
    }

    return std::ranges::all_of(paint(surfaces, area),
        std::to_underlying<totally_not_bool>);
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
