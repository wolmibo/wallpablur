#ifndef GL_SECTOR_HPP_INCLUDED
#define GL_SECTOR_HPP_INCLUDED

#include <gl/mesh.hpp>



namespace gl {

class sector {
  public:
    sector(float, size_t);



    [[nodiscard]] float  angle()      const { return angle_;      }
    [[nodiscard]] size_t resolution() const { return resolution_; }



    void draw() const { mesh_.draw(); }



  private:
    float    angle_;
    size_t   resolution_;

    gl::mesh mesh_;
};

}

#endif // GL_SECTOR_HPP_INCLUDED
