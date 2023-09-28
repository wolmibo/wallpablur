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



  [[nodiscard]] std::array<float, 4> scale_matrix(float x, float y) {
    return {
        x, 0.f,
      0.f,   y,
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

    float                    width,
    float                    height
  ) {
    switch (scale_mode) {
      case config::scale_mode::zoom: {
        auto scale = std::min<float>(width / geometry.physical_width(),
            height / geometry.physical_height());

        return scale_matrix(geometry.physical_width() * scale / width,
            geometry.physical_height() * scale / height);
      }

      case config::scale_mode::fit: {
        auto scale = std::max<float>(width / geometry.physical_width(),
            height / geometry.physical_height());

        return scale_matrix(geometry.physical_width() * scale / width,
            geometry.physical_height() * scale / height);
      }

      case config::scale_mode::centered:
        return scale_matrix(geometry.physical_width() / width,
            geometry.physical_height() / height);

      case config::scale_mode::stretch:
        return scale_matrix(1.f, 1.f);
    }

    logcerr::warn("unsupported image scale mode");
    return scale_matrix(1.f, 1.f);
  }
}





gl::texture texture_generator::create_base_texture(
  const wayland::geometry& geometry,
  const config::brush&     brush
) const {
  auto texture = load_to_gl_texture(brush.fgraph->path);
  texture.bind();
  auto size = gl::active_texture_size();

  if (static_cast<unsigned int>(size.width)  == geometry.physical_width() &&
      static_cast<unsigned int>(size.height) == geometry.physical_height() &&
      brush.solid[3] == 0.f) {
    return texture;
  }

  logcerr::verbose("rescaling image {}x{} -> {}x{} ontop of ({:.2},{:.2},{:.2},{:.2})",
      size.width, size.height,
      geometry.physical_width(), geometry.physical_height(),
      brush.solid[0], brush.solid[1], brush.solid[2], brush.solid[3]);

  gl::texture output{
    static_cast<GLsizei>(geometry.physical_width()),
    static_cast<GLsizei>(geometry.physical_height()),
    gl::texture::format::rgba8
  };

  gl::framebuffer buffer{output};
  {
    auto lock = buffer.bind();
    glViewport(0, 0, geometry.physical_width(), geometry.physical_height());

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
        scale_matrix(brush.fgraph->distribution.scale,
          geometry, size.width, size.height).data());
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
