#ifndef GL_FRAMEBUFFER_HPP_INCLUDED
#define GL_FRAMEBUFFER_HPP_INCLUDED

#include "gl/object-name.hpp"



namespace gl {

class texture;

class framebuffer {
  public:
    explicit framebuffer(const gl::texture&);
    framebuffer(const gl::texture&, const gl::texture&);



    class framebuffer_lock {
      public:
        framebuffer_lock(const framebuffer_lock &) = delete;
        framebuffer_lock(framebuffer_lock &&) noexcept;
        framebuffer_lock &operator=(const framebuffer_lock &) = delete;
        framebuffer_lock &operator=(framebuffer_lock &&) noexcept;

        framebuffer_lock() = default;
        ~framebuffer_lock();

      private:
        bool needs_unlock_{true};
    };

    [[nodiscard]] framebuffer_lock bind() const;



  private:
    struct deleter {
      void operator()(GLuint f) { glDeleteFramebuffers(1, &f); }
    };

    object_name<deleter> framebuffer_;
};

}
#endif // GL_FRAMEBUFFER_HPP_INCLUDED
