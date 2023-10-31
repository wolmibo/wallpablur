#include "gl/framebuffer.hpp"
#include "gl/texture.hpp"

#include <array>
#include <stdexcept>
#include <string_view>
#include <utility>



gl::framebuffer::framebuffer_lock::~framebuffer_lock() {
  if (needs_unlock_) {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
  }
}



gl::framebuffer::framebuffer_lock::framebuffer_lock(framebuffer_lock&& rhs) noexcept :
  needs_unlock_{std::exchange(rhs.needs_unlock_, false)}
{}



gl::framebuffer::framebuffer_lock&
gl::framebuffer::framebuffer_lock::operator=(framebuffer_lock&& rhs) noexcept
{
  needs_unlock_ = needs_unlock_ || rhs.needs_unlock_;
  rhs.needs_unlock_ = false;

  return *this;
}





gl::framebuffer::framebuffer_lock gl::framebuffer::bind() const {
  glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_.get());

  return framebuffer_lock{};
}





namespace {
  [[nodiscard]] std::string_view framebuffer_status_to_string(GLenum status) {
    switch (status) {
      case GL_FRAMEBUFFER_UNDEFINED:
        return "undefined";
      case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
        return "incomplete attachment";
      case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
        return "incomplete missing attachment";
      case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
        return "incomplete draw buffer";
      case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
        return "incomplete read buffer";
      case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
        return "incomplete multisample";
      case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
        return "incomplete layer targets";

      case 0:
        return "unable to check status";

      default:
        return "unknown status";
    }
  }



  [[nodiscard]] GLuint generate_framebuffer() {
    GLuint name{0};
    glGenFramebuffers(1, &name);
    return name;
  }



  void init_framebuffer() {
    static constexpr std::array<GLenum, 1> draw_buffers = {
      GL_COLOR_ATTACHMENT0
    };
    glDrawBuffers(draw_buffers.size(), draw_buffers.data());


    if (auto res = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        res != GL_FRAMEBUFFER_COMPLETE) {

      throw std::runtime_error{"unable to create framebuffer: "
        + std::string{framebuffer_status_to_string(res)}};
    }
  }
}



gl::framebuffer::framebuffer(const gl::texture& texture) :
  framebuffer_{generate_framebuffer()}
{
  auto fb_lock = bind();

  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
      GL_TEXTURE_2D, texture.get(), 0);

  init_framebuffer();
}



gl::framebuffer::framebuffer(const gl::texture& texture, const gl::texture& stencil) :
  framebuffer_{generate_framebuffer()}
{
  auto fb_lock = bind();

  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
      GL_TEXTURE_2D, texture.get(), 0);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
      GL_TEXTURE_2D, stencil.get(), 0);

  init_framebuffer();
}
