#include "wallpablur/config/filter.hpp"
#include "wallpablur/gl/utils.hpp"
#include "wallpablur/texture-generator.hpp"
#include "shader/shader.hpp"

#include <algorithm>
#include <array>
#include <stdexcept>
#include <utility>

#include <logcerr/log.hpp>

#include <gl/framebuffer.hpp>



[[nodiscard]] gl::texture load_to_gl_texture(const std::filesystem::path&);



namespace {
  [[nodiscard]] std::shared_ptr<egl::context> make_current(
      std::shared_ptr<egl::context> context
  ) {
    context->make_current();
    return context;
  }
}



texture_generator::texture_generator(std::shared_ptr<egl::context> context) :
  context_     {make_current(std::move(context))},
  quad_        {gl::create_quad()},
  draw_texture_{resources::rescale_texture_vs(), resources::rescale_texture_fs()}
{}





namespace {
  void setup_context(const egl::context& context) {
    context.make_current();

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
  }



  [[nodiscard]] std::array<float, 4> scale_matrix(vec2<float> scale) {
    return {
      scale.x(), 0.f,
      0.f,       -scale.y(),
    };
  }



  template<typename Enum>
  void tex_parameter(GLenum key, Enum value) {
    glTexParameteri(GL_TEXTURE_2D, key, std::to_underlying(value));
  }



  void setup_texture_parameter(const config::image_distribution& distribution) {
    tex_parameter(GL_TEXTURE_WRAP_S, distribution.wrap_x);
    tex_parameter(GL_TEXTURE_WRAP_T, distribution.wrap_y);

    tex_parameter(GL_TEXTURE_MIN_FILTER, distribution.filter);
    tex_parameter(GL_TEXTURE_MAG_FILTER, distribution.filter);
  }



  [[nodiscard]] std::array<float, 4> scale_matrix(
    config::scale_mode       scale_mode,
    const wayland::geometry& geometry,

    vec2<float>              size
  ) {
    auto physical_size = vec_cast<float>(geometry.physical_size());
    auto s = div(size, physical_size);

    switch (scale_mode) {
      case config::scale_mode::zoom:
        return scale_matrix(div(physical_size * std::min(s.x(), s.y()), size));

      case config::scale_mode::fit:
        return scale_matrix(div(physical_size * std::max(s.x(), s.y()), size));

      case config::scale_mode::centered:
        return scale_matrix(div(physical_size, size));

      case config::scale_mode::stretch:
        return scale_matrix(vec2{1.f});
    }

    logcerr::warn("unsupported image scale mode");
    return scale_matrix(vec2{1.f});
  }
}





gl::texture texture_generator::create_base_texture(
  const wayland::geometry& geometry,
  const config::brush&     brush
) const {
  auto texture = load_to_gl_texture(brush.fgraph->path);
  texture.bind();

  auto s = gl::active_texture_size();
  auto size = vec_cast<float>(vec2{s.width, s.height});

  logcerr::verbose("rescaling image {:#} -> {:#} ontop of ({:.2},{:.2},{:.2},{:.2})",
      size, geometry.physical_size(),
      brush.solid[0], brush.solid[1], brush.solid[2], brush.solid[3]);

  gl::texture output{
    static_cast<GLsizei>(geometry.physical_size().x()),
    static_cast<GLsizei>(geometry.physical_size().y()),
    gl::texture::format::rgba8
  };

  gl::framebuffer buffer{output};
  {
    auto lock = buffer.bind();
    glViewport(0, 0, geometry.physical_size().x(), geometry.physical_size().y());

    glClearColor(
        brush.solid[0] * brush.solid[3],
        brush.solid[1] * brush.solid[3],
        brush.solid[2] * brush.solid[3],
        brush.solid[3]
    );
    glClear(GL_COLOR_BUFFER_BIT);

    draw_texture_.use();
    texture.bind();
    setup_texture_parameter(brush.fgraph->distribution);
    glUniformMatrix2fv(0, 1, GL_FALSE,
        scale_matrix(brush.fgraph->distribution.scale, geometry, size).data());
    quad_.draw();
  }

  return output;
}





gl::texture texture_generator::generate_from_existing(
  const gl::texture&              texture,
  const wayland::geometry&        geometry,
  std::span<const config::filter> remaining_filters
) const {
  if (remaining_filters.empty()) {
    throw std::runtime_error{"trying to create texture which already exists"};
  }
  logcerr::verbose("creating texture from existing with {} filter(s) remaining",
      remaining_filters.size());

  setup_context(*context_);

  auto output = apply_filter(texture, remaining_filters[0], geometry);
  for (const auto& filter: remaining_filters.subspan(1)) {
    output = apply_filter(output, filter, geometry);
  }

  return output;
}



gl::texture texture_generator::generate(
  const wayland::geometry& geometry,
  const config::brush&     brush
) const {
  if (!brush.fgraph) {
    throw std::runtime_error{"missing filter_graph in brush"};
  }

  logcerr::verbose("creating texture from scratch");

  setup_context(*context_);

  auto output = create_base_texture(geometry, brush);
  for (const auto& filter: brush.fgraph->filters) {
    output = apply_filter(output, filter, geometry);
  }

  return output;
}





texture_generator::~texture_generator() {
  try {
    if (context_) {
      context_->make_current();
    }
  } catch (std::exception& ex) {
    logcerr::error(ex.what());
  }
}







namespace {
  [[nodiscard]] gl::texture line_blur(
      const gl::texture& texture,
      int                samples,
      float              dx,
      float              dy,
      float              dithering,
      const gl::program& shader,
      const gl::mesh&    quad
  ) {
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
    glUniform1f(shader.uniform("dithering"), dithering / 255.f);
    glUniform1i(shader.uniform("samples"), samples);
    glUniform2f(shader.uniform("direction"),
        dx / static_cast<float>(size.width), dy / static_cast<float>(size.height));

    quad.draw();

    return output;
  }



  [[nodiscard]] gl::texture box_blur(
      const gl::texture&             texture,
      const config::box_blur_filter& filter,
      const wayland::geometry&       /*geometry*/,
      const gl::program&             shader,
      const gl::mesh&                quad
  ) {
    if (filter.iterations == 0) {
      throw std::runtime_error{"unable to apply box blur filter with 0 iterations"};
    }

    logcerr::verbose("applying box blur filter with scale {}x{} and {} iterations",
        filter.width * 2 + 1, filter.height * 2 + 1, filter.iterations);

    auto output = line_blur(line_blur(texture, filter.width, 1.f, 0.f,
                                      filter.dithering, shader, quad),
                    filter.height, 0.f, 1.f, filter.dithering, shader, quad);

    for (unsigned int i = 1; i < filter.iterations; ++i) {
      output = line_blur(line_blur(output, filter.width, 1.f, 0.f,
                                   filter.dithering, shader, quad),
                    filter.height, 0.f, 1.f, filter.dithering, shader, quad);
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
        filter_shader_cache_.find_or_create(shader::box_blur,
          resources::filter_vs(), resources::filter_line_blur_fs()),
        quad_);
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
      logcerr::verbose("applying invert filter");
      filter_shader_cache_.find_or_create(shader::invert,
          resources::filter_vs(), resources::filter_invert_fs()).use();

    } else {
      throw std::runtime_error{"trying to use unimplemented filter"};
    }
    quad_.draw();
  }

  return output;
}
