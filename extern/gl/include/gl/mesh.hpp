#ifndef GL_MESH_HPP_INCLUDED
#define GL_MESH_HPP_INCLUDED

#include "gl/object-name.hpp"



namespace gl {

class mesh {
  public:
    mesh() = default;
    mesh(GLuint, GLuint, GLuint);



    void draw() const;



  private:
    struct buffer_deleter {
      void operator()(GLuint b) { glDeleteBuffers(1, &b); }
    };

    struct va_deleter {
      void operator()(GLuint v) { glDeleteVertexArrays(1, &v); }
    };

    [[nodiscard]] static size_t get_element_count(GLuint);



    object_name<va_deleter>     vao_;
    object_name<buffer_deleter> vbo_;
    object_name<buffer_deleter> ibo_;

    size_t element_count_{0};
};

}

#endif // GL_MESH_HPP_INCLUDED
