#include "wallpablur/rectangle.hpp"



std::array<float, 16> rectangle::to_matrix(float width, float height) const {
  vec2 size{width, height};

  auto scale = div(size_, size);

  auto pos = div(mul((2.f * pos_ - (size - size_)), vec2{1.f, -1.f}), size);

  auto a = scale.x();  auto b = 0.f;
  auto c = 0.f;        auto d = scale.y();

  switch (rot_cw90_ % 4) {
    case 0:
    default:
      break;
    case 2:
      a = -scale.x();
      d = -scale.y();
      break;
    case 1:
      a = 0.f;
      b = -scale.y();
      c = scale.x();
      d = 0.f;
      break;
    case 3:
      a = 0.f;
      b = scale.y();
      c = -scale.x();
      d = 0.f;
      break;
  }

  return {
          a,       b, 0.f, 0.f,
          c,       d, 0.f, 0.f,
        0.f,     0.f, 1.f, 0.f,
    pos.x(), pos.y(), 0.f, 1.f,
  };
}
