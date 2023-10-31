#include "gl/texture.hpp"

#include <stdexcept>

#include <cassert>


namespace {
  [[nodiscard]] GLint format_to_internal(gl::texture::format format) {
    switch (format) {
      case gl::texture::format::rgb8:             return GL_RGB8;
      case gl::texture::format::rgba8:            return GL_RGBA8;
      case gl::texture::format::depth24_stencil8: return GL_DEPTH24_STENCIL8;
    }
    throw std::runtime_error{"unsupported texture format"};
  }



  [[nodiscard]] GLenum format_to_format(gl::texture::format format) {
    switch (format) {
      case gl::texture::format::rgb8:             return GL_RGB;
      case gl::texture::format::rgba8:            return GL_RGBA;
      case gl::texture::format::depth24_stencil8: return GL_DEPTH_STENCIL;
    }
    throw std::runtime_error{"unsupported texture format"};
  }



  [[nodiscard]] GLenum format_to_type(gl::texture::format format) {
    switch (format) {
      case gl::texture::format::depth24_stencil8: return GL_UNSIGNED_INT_24_8;
      case gl::texture::format::rgb8:
      case gl::texture::format::rgba8:            return GL_UNSIGNED_BYTE;
    }
    throw std::runtime_error{"unsupported texture format"};
  }



  [[nodiscard]] GLint unpack_alignment(size_t stride) {
    GLint alignment = 8;

    while (stride % alignment != 0) {
      alignment >>= 1;
    }

    return alignment;
  }



  [[nodiscard]] GLint pixels_per_stride(size_t stride, gl::texture::format format) {
    switch (format) {
      case gl::texture::format::depth24_stencil8: return stride / 4;
      case gl::texture::format::rgb8:             return stride / 3;
      case gl::texture::format::rgba8:            return stride / 4;
    }
    throw std::runtime_error{"unsupported texture format"};
  }





  [[nodiscard]] GLuint generate_texture() {
    GLuint texture{0};
    glGenTextures(1, &texture);
    return texture;
  }
}



gl::texture::texture() :
  texture_{generate_texture()}
{}



gl::texture::texture(texture_size size, format fmt) :
  texture{size.width, size.height, fmt}
{}



gl::texture::texture(GLsizei width, GLsizei height, format fmt) :
  texture{}
{
  bind();

  glTexImage2D(GL_TEXTURE_2D, 0, format_to_internal(fmt),
      width, height, 0,
      format_to_format(fmt),
      format_to_type(fmt), nullptr);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

  unbind();
}



gl::texture::texture(
    GLsizei                    width,
    GLsizei                    height,
    std::span<const std::byte> data,
    format                     fmt
) :
  texture{}
{
  bind();

  size_t stride = data.size() / height;
  assert(stride * height == data.size());

  glPixelStorei(GL_UNPACK_ALIGNMENT,  unpack_alignment(stride));
  glPixelStorei(GL_UNPACK_ROW_LENGTH, pixels_per_stride(stride, fmt));

  glTexImage2D(GL_TEXTURE_2D, 0, format_to_internal(fmt),
      width, height, 0,
      format_to_format(fmt),
      format_to_type(fmt), data.data());

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

  unbind();
}





gl::texture_size gl::active_texture_size() {
  texture_size out;
  glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH,  &out.width);
  glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &out.height);
  return out;
}
