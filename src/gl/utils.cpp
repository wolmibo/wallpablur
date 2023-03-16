#include "wallpablur/gl/utils.hpp"



namespace {
  [[nodiscard]] GLuint generate_vertex_array() {
    GLuint vao{0};

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    return vao;
  }



  [[nodiscard]] GLuint generate_vertex_buffer(std::span<const GLfloat> vertices) {
    GLuint vbo{0};

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER,
        vertices.size_bytes(), vertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), nullptr);
    glEnableVertexAttribArray(0);

    return vbo;
  }



  [[nodiscard]] GLuint generate_index_buffer(std::span<const GLushort> indices) {
    GLuint ibo{0};

    glGenBuffers(1, &ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
        indices.size_bytes(), indices.data(), GL_STATIC_DRAW);

    return ibo;
  }
}



gl::mesh gl::mesh_from_vertices_indices(
    std::span<const GLfloat>  vertices,
    std::span<const GLushort> indices
) {
  return gl::mesh{
    generate_vertex_array(),
    generate_vertex_buffer(vertices),
    generate_index_buffer(indices)
  };
}
