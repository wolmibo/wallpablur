#include "wallpablur/rectangle.hpp"



std::array<float, 16> rectangle::to_matrix(float width, float height) const {
  auto sx = width_  / width;
  auto sy = height_ / height;

  auto dx = width  - width_;
  auto dy = height - height_;

  auto x = 2.f * x_ - dx;
  auto y = dy - 2.f * y_;

  auto e = x / width;
  auto f = y / height;

  auto a = sx;  auto b = 0.f;
  auto c = 0.f; auto d = sy;

  switch (rot_cw90_ % 4) {
    case 0:
    default:
      break;
    case 2:
      a = -sx;
      d = -sy;
      break;
    case 1:
      a = 0.f;
      b = -sy;
      c = sx;
      d = 0.f;
      break;
    case 3:
      a = 0.f;
      b = sy;
      c = -sx;
      d = 0.f;
      break;
  }

  return {
      a,   b, 0.f, 0.f,
      c,   d, 0.f, 0.f,
    0.f, 0.f, 1.f, 0.f,
      e,   f, 0.f, 1.f,
  };
}
