#include "wallpablur/layout-painter.hpp"
#include "shader/shader.hpp"

#include <array>

#include <logging/log.hpp>



namespace {
  using mat4 = std::array<GLfloat, 16>;



  constexpr mat4 mat4_unity {
    1.f, 0.f, 0.f, 0.f,
    0.f, 1.f, 0.f, 0.f,
    0.f, 0.f, 1.f, 0.f,
    0.f, 0.f, 0.f, 1.f
  };



  constexpr mat4 rectangle_to_matrix(rectangle in, GLfloat width, GLfloat height) {
    auto sx = static_cast<GLfloat>(in.width)  / width;
    auto sy = static_cast<GLfloat>(in.height) / height;

    auto dx = (width  - static_cast<GLfloat>(in.width));
    auto dy = (height - static_cast<GLfloat>(in.height));

    auto x = 2.f * static_cast<GLfloat>(in.x) - dx;
    auto y = dy - 2.f * static_cast<GLfloat>(in.y);

    auto a = x / width;
    auto b = y / height;

    return {
       sx, 0.f, 0.f, 0.f,
      0.f,  sy, 0.f, 0.f,
      0.f, 0.f, 1.f, 0.f,
        a,   b, 0.f, 1.f,
    };
  }





  template<typename MASK, typename DRAW>
  void draw_stenciled(MASK mask_fnc, DRAW draw_fnc) {
    struct guard {
      guard(const guard&) = delete;
      guard(guard&&)      = delete;
      guard& operator=(const guard&) = delete;
      guard& operator=(guard&&)      = delete;

      guard() {
        glEnable(GL_STENCIL_TEST);
      }

      ~guard() {
        glDisable(GL_STENCIL_TEST);
      }
    } stencil_guard;

    glStencilFunc(GL_ALWAYS, 1, 0xFF);
    glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
    glStencilMask(0xFF);
    glClear(GL_STENCIL_BUFFER_BIT);

    mask_fnc();

    glStencilFunc(GL_EQUAL, 1, 0xFF);
    glStencilMask(0x00);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    draw_fnc();
  }





  template<typename FNC, typename ...ARGS>
  void invoke_append_color(FNC&& f, const config::color& c, float a, ARGS&&... args) {
    std::forward<FNC>(f)(std::forward<ARGS>(args)...,
        c[0] * c[3] * a, c[1] * c[3] * a, c[2] * c[3] * a, c[3] * a);
  }





  std::shared_ptr<egl::context> assert_non_nullptr(std::shared_ptr<egl::context>&& cx) {
    if (!cx) {
      throw std::runtime_error{"expected non-null context"};
    }
    return cx;
  }





  template<typename T>
  T saturate_sub(T a, T b) {
    if (b > a) {
      return 0;
    }
    return a - b;
  }



  surface panel_to_surface(const config::panel& panel, const wayland::geometry& geo) {
    using an = config::panel::anchor_type;

    auto anchor = panel.anchor;
    auto size   = panel.size;
    auto margin = panel.margin;

    rectangle rect;

    if (size.width == 0 &&
        (anchor.value & an::left) != 0 && (anchor.value & an::right) != 0) {

      rect.width = saturate_sub<uint32_t>(geo.logical_width(),
                      margin.left + margin.right);
    } else {
      rect.width = size.width;
    }

    if (size.height == 0 &&
        (anchor.value & an::top) != 0 && (anchor.value & an::bottom) != 0) {

      rect.height = saturate_sub<uint32_t>(geo.logical_height(),
                      margin.top + margin.bottom);
    } else {
      rect.height = size.height;
    }



    if ((anchor.value & an::left) != 0 && (anchor.value & an::right) == 0) {
      rect.x = margin.left;
    } else if ((anchor.value & an::right) != 0 && (anchor.value & an::left) == 0) {
      rect.x = saturate_sub(geo.logical_width(), margin.right + rect.width);
    } else {
      rect.x = saturate_sub(geo.logical_width() / 2,
          (rect.width + margin.left + margin.right) / 2) + margin.left;
    }

    if ((anchor.value & an::top) != 0 && (anchor.value & an::bottom) == 0) {
      rect.y = margin.top;
    } else if ((anchor.value & an::bottom) != 0 && (anchor.value & an::top) == 0) {
      rect.y = saturate_sub(geo.logical_height(), margin.bottom + rect.height);
    } else {
      rect.y = saturate_sub(geo.logical_height() / 2,
          (rect.height + margin.bottom + margin.top) / 2) + margin.top;
    }

    if (!geo.empty()) {
      if (rect.width * rect.height == 0) {
        logging::warn("fixed panel has size {}x{}", rect.width, rect.height);
      }
    }

    return surface{rect, surface_type::panel};
  }
}





layout_painter::layout_painter(
  config::output&&                  config,
  std::shared_ptr<egl::context>     context,
  std::shared_ptr<texture_provider> provider
) :
  config_             {std::move(config)},
  context_            {assert_non_nullptr(std::move(context))},
  texture_provider_   {std::move(provider)},
  quad_               {[](const auto& cx) {
                         cx.make_current();
                         return gl::plane{};
                       }(*context_)},
  solid_color_shader_ {resources::solid_color_vs(), resources::solid_color_fs()},
  solid_color_uniform_{solid_color_shader_.uniform("color_rgba")},

  texture_shader_     {resources::texture_vs(), resources::texture_fs()},
  texture_a_uniform_  {texture_shader_.uniform("alpha")}
{}





bool layout_painter::update_geometry(const wayland::geometry& geometry) {
  if (geometry == geometry_) {
    return false;
  }

  geometry_ = geometry;

  logging::verbose("{}: update geometry to logical = {}x{}@{}, pixel = {}x{}",
      config_.name,
      geometry_.logical_width(), geometry_.logical_height(), geometry_.scale(),
      geometry_.pixel_width(), geometry_.pixel_height());



  fixed_panels_.clear();
  fixed_panels_.reserve(config_.fixed_panels.size());

  for (const auto& panel : config_.fixed_panels) {
    fixed_panels_.push_back(panel_to_surface(panel, geometry_));
  }



  if (config_.wallpaper.fgraph) {
    wallpaper_ = texture_provider_->get(geometry, config_.wallpaper);

    if (!wallpaper_) {
      logging::warn("{}: retrieved empty wallpaper image", config_.name);
    }
  }

  if (config_.background.fgraph) {
    background_ = texture_provider_->get(geometry, config_.background);
    if (!background_) {
      logging::warn("{}: retrieved empty background image", config_.name);
    }
  }

  texture_provider_->cleanup();

  return true;
}





void layout_painter::draw_rectangle(const rectangle& rect) const {
  glUniformMatrix4fv(0, 1, GL_FALSE,
      rectangle_to_matrix(rect,
        geometry_.logical_width(), geometry_.logical_height()).data());

  quad_.draw();
}





void layout_painter::draw_layout(
  const wm::layout&   layout,
  float               alpha
) const {
  logging::debug("{}: rendering", config_.name);
  context_->make_current();

  glViewport(0, 0, geometry_.pixel_width(), geometry_.pixel_height());


  if (wallpaper_) {
    glClearColor(0.f, 0.f, 0.f, 0.f);
    glClear(GL_COLOR_BUFFER_BIT);

    texture_shader_.use();
    glUniform1f(texture_a_uniform_, alpha);

    wallpaper_->bind();
    quad_.draw();

  } else {
    invoke_append_color(glClearColor, config_.wallpaper.solid, alpha);
    glClear(GL_COLOR_BUFFER_BIT);
  }


  draw_stenciled([&]() {
    solid_color_shader_.use();

    for (const auto& surface: layout) {
      draw_rectangle(surface.rect());
    }

    for (const auto& surface: fixed_panels_) {
      draw_rectangle(surface.rect());
    }

  }, [&](){
    if (background_) {
      texture_shader_.use();
      glUniform1f(texture_a_uniform_, alpha);

      background_->bind();
      quad_.draw();

    } else {
      solid_color_shader_.use();

      invoke_append_color(glUniform4f,
          config_.background.solid, alpha, solid_color_uniform_);

      glUniformMatrix4fv(0, 1, GL_FALSE, mat4_unity.data());
      quad_.draw();
    }
  });
}





layout_painter::~layout_painter() {
  try {
    if (context_) {
      context_->make_current();
    }
  } catch (std::exception& ex) {
    logging::error(ex.what());
  }
}
