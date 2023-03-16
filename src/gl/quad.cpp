#include "wallpablur/gl/quad.hpp"

#include <array>



namespace {
  constexpr std::array<GLfloat, 16> vertices {
    -1.f,  1.f, 0.f, 1.f,
     1.f,  1.f, 0.f, 1.f,
    -1.f, -1.f, 0.f, 1.f,
     1.f, -1.f, 0.f, 1.f
  };

  constexpr std::array<GLushort, 6> indices {
    0, 1, 2,
    1, 2, 3,
  };



  template<typename T, std::size_t S>
  [[nodiscard]] constexpr std::size_t byte_size(const std::array<T, S>& /*unused*/) {
    return sizeof(T) * S;
  }



  [[nodiscard]] GLuint generate_vertex_array() {
    GLuint vao{0};

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    return vao;
  }



  [[nodiscard]] GLuint generate_vertex_buffer() {
    GLuint vbo{0};

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, byte_size(vertices), vertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), nullptr);
    glEnableVertexAttribArray(0);

    return vbo;
  }



  [[nodiscard]] GLuint generate_index_buffer() {
    GLuint ibo{0};

    glGenBuffers(1, &ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
        byte_size(indices), indices.data(), GL_STATIC_DRAW);

    return ibo;
  }
}



gl::quad::quad() :
  mesh_{generate_vertex_array(), generate_vertex_buffer(), generate_index_buffer()}
{}
