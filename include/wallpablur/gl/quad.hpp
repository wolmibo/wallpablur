#ifndef GL_QUAD_HPP_INCLUDED
#define GL_QUAD_HPP_INCLUDED

#include <gl/mesh.hpp>



namespace gl {

class quad {
  public:
    quad();

    void draw() const { mesh_.draw(); }



  private:
    gl::mesh mesh_;
};

}

#endif // GL_QUAD_HPP_INCLUDED
