#ifndef GL_TEXTURE_HPP_INCLUDED
#define GL_TEXTURE_HPP_INCLUDED

#include "gl/object-name.hpp"

#include <span>



namespace gl {

struct texture_size {
  GLsizei width {0};
  GLsizei height{0};
};



class texture {
  public:
    enum class format {
      rgba8,
      rgb8,
    };



    texture();
    texture(texture_size, format = format::rgba8);
    texture(GLsizei, GLsizei, format = format::rgba8);
    texture(GLsizei, GLsizei, std::span<const std::byte>, format = format::rgba8);



    [[nodiscard]] GLuint get()    const { return texture_.get();    }
    void                 bind()   const { bind_tex(texture_.get()); }
    static void          unbind()       { bind_tex(0);              }



  private:
    struct deleter {
      void operator()(GLuint t) { glDeleteTextures(1, &t); }
    };

    object_name<deleter> texture_;


    static void bind_tex(GLuint t) {
      glBindTexture(GL_TEXTURE_2D, t);
    }
};



[[nodiscard]] texture_size active_texture_size();

}

#endif // GL_TEXTURE_HPP_INCLUDED
