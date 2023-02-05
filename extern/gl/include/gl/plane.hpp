#ifndef GL_PLANE_HPP_INCLUDED
#define GL_PLANE_HPP_INCLUDED

#include "gl/object-name.hpp"



namespace gl {

class plane {
  public:
    plane();

    void draw() const;



  private:
    struct buffer_deleter {
      void operator()(GLuint b) { glDeleteBuffers(1, &b); }
    };
    struct va_deleter {
      void operator()(GLuint v) { glDeleteVertexArrays(1, &v); }
    };



    object_name<va_deleter>     vao_;
    object_name<buffer_deleter> vbo_;
    object_name<buffer_deleter> ibo_;
};

}

#endif // GL_PLANE_HPP_INCLUDED
