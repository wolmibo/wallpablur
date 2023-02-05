#include "wallpablur/config/filter.hpp"
#include "wallpablur/texture-generator.hpp"
#include "shader/shader.hpp"

#include <algorithm>
#include <array>
#include <stdexcept>

#include <logging/log.hpp>

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
  draw_texture_{resources::rescale_texture_vs(), resources::rescale_texture_fs()}
{}



void texture_generator::setup_context() const {
  context_->make_current();
  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
}



namespace {
  std::array<float, 4> scale_matrix(float x, float y) {
    return {
        x, 0.f,
      0.f,   y,
    };
  }



  std::array<float, 4> setup_texture_distribution(
    const config::image_distribution& distribution,
    const wayland::geometry&          geometry,

    float width, float height
  ) {
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
        std::to_underlying(distribution.wrap_x));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
        std::to_underlying(distribution.wrap_y));

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    switch (distribution.scale) {
      case config::scale_mode::zoom: {
        auto scale = std::min<float>(width / geometry.pixel_width(),
            height / geometry.pixel_height());

        return scale_matrix(geometry.pixel_width() * scale / width,
            geometry.pixel_height() * scale / height);
      }

      case config::scale_mode::fit: {
        auto scale = std::max<float>(width / geometry.pixel_width(),
            height / geometry.pixel_height());

        return scale_matrix(geometry.pixel_width() * scale / width,
            geometry.pixel_height() * scale / height);
      }

      case config::scale_mode::centered:
        return scale_matrix(geometry.pixel_width() / width,
            geometry.pixel_height() / height);

      case config::scale_mode::stretch:
        return scale_matrix(1.f, 1.f);
    }

    logging::warn("unsupported image scale mode");
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

  if (static_cast<unsigned int>(size.width)  == geometry.pixel_width() &&
      static_cast<unsigned int>(size.height) == geometry.pixel_height() &&
      brush.solid[3] == 0.f) {
    return texture;
  }

  logging::verbose("rescaling image {}x{} -> {}x{} ontop of ({:.2},{:.2},{:.2},{:.2})",
      size.width, size.height,
      geometry.pixel_width(), geometry.pixel_height(),
      brush.solid[0], brush.solid[1], brush.solid[2], brush.solid[3]);

  gl::texture output{
    static_cast<GLsizei>(geometry.pixel_width()),
    static_cast<GLsizei>(geometry.pixel_height()),
    gl::texture::format::rgba8
  };

  gl::framebuffer buffer{output};
  {
    auto lock = buffer.bind();
    glViewport(0, 0, geometry.pixel_width(), geometry.pixel_height());

    glClearColor(
        brush.solid[0] * brush.solid[3],
        brush.solid[1] * brush.solid[3],
        brush.solid[2] * brush.solid[3],
        brush.solid[3]
    );
    glClear(GL_COLOR_BUFFER_BIT);

    draw_texture_.use();
    texture.bind();
    glUniformMatrix2fv(0, 1, GL_FALSE,
        setup_texture_distribution(brush.fgraph->distribution,
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
  logging::verbose("creating texture from existing with {} filter(s) remaining",
      remaining_filters.size());

  setup_context();

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

  logging::verbose("creating texture from scratch");

  setup_context();

  auto output = create_base_texture(geometry, brush);
  for (const auto& filter: brush.fgraph->filters) {
    output = apply_filter(output, filter, geometry);
  }

  return output;
}



const gl::program& texture_generator::filter_shader(
  std::string_view key,
  std::string_view fs
) const {
  return filter_shader_cache_.find_or_create(key, resources::filter_vs(), fs);
}



texture_generator::~texture_generator() {
  try {
    if (context_) {
      context_->make_current();
    }
  } catch (std::exception& ex) {
    logging::error(ex.what());
  }
}
