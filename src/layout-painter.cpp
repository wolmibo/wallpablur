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



  constexpr mat4 rectangle_to_matrix(rect in, GLfloat width, GLfloat height) {
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



  rect panel_to_rectangle(const config::panel& panel, const wayland::geometry& geo) {
    using an = config::panel::anchor_type;

    auto anchor = panel.anchor;
    auto size   = panel.size;
    auto margin = panel.margin;

    rect out;

    if (size.width == 0 &&
        (anchor.value & an::left) != 0 && (anchor.value & an::right) != 0) {

      out.width = saturate_sub<uint32_t>(geo.width, margin.left + margin.right);
    } else {
      out.width = size.width;
    }

    if (size.height == 0 &&
        (anchor.value & an::top) != 0 && (anchor.value & an::bottom) != 0) {

      out.height = saturate_sub<uint32_t>(geo.height, margin.top + margin.bottom);
    } else {
      out.height = size.height;
    }



    if ((anchor.value & an::left) != 0 && (anchor.value & an::right) == 0) {
      out.x = margin.left;
    } else if ((anchor.value & an::right) != 0 && (anchor.value & an::left) == 0) {
      out.x = saturate_sub(geo.width, margin.right + out.width);
    } else {
      out.x = saturate_sub(geo.width / 2,
          (out.width + margin.left + margin.right) / 2) + margin.left;
    }

    if ((anchor.value & an::top) != 0 && (anchor.value & an::bottom) == 0) {
      out.y = margin.top;
    } else if ((anchor.value & an::bottom) != 0 && (anchor.value & an::top) == 0) {
      out.y = saturate_sub(geo.height, margin.bottom + out.height);
    } else {
      out.y = saturate_sub(geo.height / 2,
          (out.height + margin.bottom + margin.top) / 2) + margin.top;
    }

    if (geo.width * geo.height != 0) {
      if (out.width * out.height == 0) {
        logging::warn("fixed panel has size {}x{}", out.width, out.height);
      }
    }

    return out;
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

  logging::verbose("{}: changed output geometry to {}x{}@{}",
      config_.name, geometry_.width, geometry_.height, geometry_.scale);

  fixed_panels_.resize(config_.fixed_panels.size());
  for (size_t i = 0; i < config_.fixed_panels.size(); ++i) {
    fixed_panels_[i] = panel_to_rectangle(config_.fixed_panels[i], geometry_);
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





void layout_painter::draw_rectangle(const rect& rectangle) const {
  glUniformMatrix4fv(0, 1, GL_FALSE,
      rectangle_to_matrix(rectangle, geometry_.width, geometry_.height).data());

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

    for (const auto& rectangle: layout) {
      draw_rectangle(rectangle);
    }

    for (const auto& rectangle: fixed_panels_) {
      draw_rectangle(rectangle);
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
