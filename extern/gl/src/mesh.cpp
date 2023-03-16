#include "gl/mesh.hpp"

#include <stdexcept>





gl::mesh::mesh(GLuint vao, GLuint vbo, GLuint ibo) :
  vao_{vao},
  vbo_{vbo},
  ibo_{ibo},

  element_count_{get_element_count(ibo)}
{}





void gl::mesh::draw() const {
  glBindVertexArray(vao_.get());
  glDrawElements(GL_TRIANGLES, element_count_, GL_UNSIGNED_SHORT, nullptr);
}





size_t gl::mesh::get_element_count(GLuint ibo) {
  GLint64 byte_size{0};

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
  glGetBufferParameteri64v(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &byte_size);

  if (byte_size < 0) {
    throw std::runtime_error{"unable to obtain element count"};
  }

  if (byte_size % sizeof(GLushort) != 0) {
    throw std::runtime_error{"element count is not an integer"};
  }

  return byte_size / sizeof(GLushort);
}
