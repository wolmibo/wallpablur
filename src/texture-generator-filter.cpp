#include "wallpablur/config/filter.hpp"
#include "wallpablur/texture-generator.hpp"
#include "wallpablur/wayland/geometry.hpp"
#include "shader/shader.hpp"

#include <stdexcept>
#include <variant>

#include <gl/program.hpp>
#include <gl/texture.hpp>
#include <gl/framebuffer.hpp>

#include <logging/log.hpp>



namespace {
  gl::texture line_blur(const gl::texture& texture, int samples, float dx, float dy,
      const gl::program& shader, const gl::plane& quad) {


    texture.bind();
    auto size = gl::active_texture_size();
    gl::texture output{size};
    gl::framebuffer fb{output};

    auto lock = fb.bind();

    glViewport(0, 0, size.width, size.height);

    texture.bind();

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);

    shader.use();
    glUniform1i(shader.uniform("samples"), samples);
    glUniform2f(shader.uniform("direction"),
        dx / static_cast<float>(size.width), dy / static_cast<float>(size.height));

    quad.draw();

    return output;
  }



  gl::texture box_blur(const gl::texture& texture, const config::box_blur_filter& filter,
      const wayland::geometry& /*geometry*/,
      const gl::program& shader, const gl::plane& quad
  ) {
    if (filter.iterations == 0) {
      throw std::runtime_error{"unable to apply box blur filter with 0 iterations"};
    }

    logging::verbose("applying box blur filter with scale {}x{} and {} iterations",
        filter.width * 2 + 1, filter.height * 2 + 1, filter.iterations);

    auto output = line_blur(line_blur(texture, filter.width, 1.f, 0.f, shader, quad),
                    filter.height, 0.f, 1.f, shader, quad);

    for (unsigned int i = 1; i < filter.iterations; ++i) {
      output = line_blur(line_blur(output, filter.width, 1.f, 0.f, shader, quad),
                    filter.height, 0.f, 1.f, shader, quad);
    }
    return output;
  }
}



gl::texture texture_generator::apply_filter(
  const gl::texture&       texture,
  const config::filter&    filter,
  const wayland::geometry& geometry
) const {
  if (const auto *bblur = std::get_if<config::box_blur_filter>(&filter)) {
    return box_blur(texture, *bblur, geometry,
        filter_shader("box-blur", resources::filter_line_blur_fs()), quad_);
  }


  texture.bind();
  auto size = gl::active_texture_size();
  gl::texture output{size};
  gl::framebuffer fb{output};
  {
    auto lock = fb.bind();
    glViewport(0, 0, size.width, size.height);

    texture.bind();

    if (std::holds_alternative<config::invert_filter>(filter)) {
      logging::verbose("applying invert filter");
      filter_shader("invert", resources::filter_invert_fs()).use();

    } else {
      throw std::runtime_error{"trying to use unimplemented filter"};
    }
    quad_.draw();
  }

  return output;
}
