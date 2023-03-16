#include "wallpablur/gl/sector.hpp"
#include "wallpablur/gl/utils.hpp"

#include <vector>

#include <cmath>



namespace {
  void append_vec2(std::vector<GLfloat>& con, GLfloat x, GLfloat y) {
    con.push_back(x);
    con.push_back(y);
    con.push_back(0.f);
    con.push_back(1.f);
  }



  [[nodiscard]] std::vector<GLfloat> triangle_fan_vertices(float angle, size_t res) {
    std::vector<GLfloat> output;
    output.reserve(4 * (2 + res));

    append_vec2(output, 0, 0);

    for (size_t i = 0; i <= res; ++i) {
      float phi = static_cast<float>(i) / static_cast<float>(res) * angle;
      append_vec2(output, std::cos(phi), std::sin(phi));
    }

    return output;
  }



  [[nodiscard]] std::vector<GLushort> triangle_fan_indices(size_t res) {
    std::vector<GLushort> output;
    output.reserve(3 * res);

    for (size_t i = 0; i < res; ++i) {
      output.push_back(0);
      output.push_back(i + 1);
      output.push_back(i + 2);
    }

    return output;
  }
}





gl::sector::sector(float angle, size_t resolution) :
  angle_     {angle},
  resolution_{resolution},

  mesh_{mesh_from_vertices_indices(triangle_fan_vertices(angle, resolution),
          triangle_fan_indices(resolution))}
{}
