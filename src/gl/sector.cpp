#include "wallpablur/gl/sector.hpp"
#include "wallpablur/gl/utils.hpp"

#include <numbers>
#include <vector>

#include <cmath>



namespace {
  void append_vec2_weight(std::vector<GLfloat>& con, GLfloat x, GLfloat y, GLfloat w) {
    con.push_back(x);
    con.push_back(y);
    con.push_back(w);
    con.push_back(1.f);
  }



  [[nodiscard]] std::vector<GLfloat> triangle_fan_vertices(size_t res) {
    std::vector<GLfloat> output;
    output.reserve(4 * (2 + res));

    append_vec2_weight(output, -1, -1, 1);

    for (size_t i = 0; i <= res; ++i) {
      float phi = static_cast<float>(i) / static_cast<float>(res) * std::numbers::pi / 2;
      append_vec2_weight(output, 2 * std::cos(phi) - 1, 2 * std::sin(phi) - 1, 0);
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





gl::sector::sector(size_t resolution) :
  resolution_{resolution},

  mesh_{mesh_from_vertices_indices(triangle_fan_vertices(resolution),
          triangle_fan_indices(resolution))}
{}
