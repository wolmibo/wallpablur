#include "wallpablur/rectangle.hpp"



std::array<float, 16> rectangle::to_matrix(float width, float height) const {
  auto sx = width_  / width;
  auto sy = height_ / height;

  auto dx = width  - width_;
  auto dy = height - height_;

  auto x = 2.f * x_ - dx;
  auto y = dy - 2.f * y_;

  auto a = x / width;
  auto b = y / height;

  return {
     sx, 0.f, 0.f, 0.f,
    0.f,  sy, 0.f, 0.f,
    0.f, 0.f, 1.f, 0.f,
      a,   b, 0.f, 1.f,
  };
}
