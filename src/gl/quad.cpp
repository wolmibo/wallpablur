#include "wallpablur/gl/utils.hpp"

#include <array>



namespace {
  constexpr std::array<GLfloat, 16> vertices {
    -1.f,  1.f, 0.f, 1.f,
     1.f,  1.f, 0.f, 1.f,
    -1.f, -1.f, 1.f, 1.f,
     1.f, -1.f, 1.f, 1.f,
  };

  constexpr std::array<GLushort, 6> indices {
    0, 1, 2,
    1, 2, 3,
  };
}





gl::mesh gl::create_quad() {
  return mesh_from_vertices_indices(vertices, indices);
}
