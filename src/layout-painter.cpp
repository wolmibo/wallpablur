#include "wallpablur/config/border-effect.hpp"
#include "wallpablur/layout-painter.hpp"
#include "shader/shader.hpp"

#include <algorithm>
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
  void invoke_append_color(FNC&& f, const config::color& c, ARGS&&... args) {
    std::forward<FNC>(f)(std::forward<ARGS>(args)...,
        c[0] * c[3], c[1] * c[3], c[2] * c[3], c[3]);
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

  template<typename T, typename S>
  void saturate_dec(T& a, S b) {
    T c = static_cast<T>(std::max<S>(b, 0));
    if (c > a) {
      a = 0;
    } else {
      a -= c;
    }
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

    return surface{rect, surface_type::panel, panel.app_id, panel.focused, panel.urgent};
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

  texture_shader_     {resources::texture_vs(), resources::texture_fs()}
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





namespace {
  void set_blend_mode(config::blend_mode mode) {
    switch (mode) {
      case config::blend_mode::add:
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE);
        break;

      case config::blend_mode::alpha:
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        break;

      case config::blend_mode::replace:
        glDisable(GL_BLEND);
        break;
    }
  }



  rectangle center_tile(rectangle rect, const config::border_effect& effect) {
    rect.x += effect.offset.x;
    rect.y += effect.offset.y;

    switch (effect.position) {
      case config::border_position::inside:
        rect.x += effect.thickness;
        rect.y += effect.thickness;
        saturate_dec(rect.width, 2 * effect.thickness);
        saturate_dec(rect.height, 2 * effect.thickness);
        break;
      case config::border_position::centered:
        rect.x += effect.thickness / 2;
        rect.y += effect.thickness / 2;
        saturate_dec(rect.width,  effect.thickness);
        saturate_dec(rect.height, effect.thickness);
        break;
      case config::border_position::outside:
        break;
    }

    return rect;
  }



  [[nodiscard]] rectangle left(rectangle rect, int amount) {
    rect.x    -= amount;
    rect.width = amount;
    return rect;
  }

  [[nodiscard]] rectangle right(rectangle rect, int amount) {
    rect.x    += rect.width;
    rect.width = amount;
    return rect;
  }

  [[nodiscard]] rectangle top(rectangle rect, int amount) {
    rect.y     -= amount;
    rect.height = amount;
    return rect;
  }

  [[nodiscard]] rectangle bottom(rectangle rect, int amount) {
    rect.y     += rect.height;
    rect.height = amount;
    return rect;
  }
}



void layout_painter::draw_border_element(float x, float y, const rectangle& rect) const {
  glUniform2f(1, x, -y);
  draw_rectangle(rect);
}



void layout_painter::draw_border_effect(
    const config::border_effect& effect,
    const surface&               surf
) const {
  set_blend_mode(effect.blend);

  switch (effect.foff) {
    case config::falloff::step: {
      shader_cache_.find_or_create(shader::border_step,
          resources::border_vs(), resources::border_step_fs()).use();

      float m = std::max<uint32_t>(effect.thickness, 1);
      glUniform1f(30, 0.75f / m);
      break;
    }
    case config::falloff::linear:
      shader_cache_.find_or_create(shader::border_linear,
          resources::border_vs(), resources::border_linear_fs()).use();
      glUniform1f(20, effect.exponent);
      break;
    case config::falloff::sinusoidal:
      shader_cache_.find_or_create(shader::border_sinusoidal,
          resources::border_vs(), resources::border_sinusoidal_fs()).use();
      glUniform1f(20, effect.exponent);
      break;
  }

  invoke_append_color(glUniform4f, effect.col, 10);

  auto center = center_tile(surf.rect(), effect);

  draw_border_element(0.f, 0.f, center);

  if (auto t = effect.thickness; t > 0) {
    draw_border_element(-1.f,  0.f, left(center, t));
    draw_border_element(-1.f, -1.f, top(left(center, t), t));
    draw_border_element( 0.f, -1.f, top(center, t));
    draw_border_element( 1.f, -1.f, right(top(center, t), t));
    draw_border_element( 1.f,  0.f, right(center, t));
    draw_border_element( 1.f,  1.f, bottom(right(center, t), t));
    draw_border_element( 0.f,  1.f, bottom(center, t));
    draw_border_element(-1.f,  1.f, left(bottom(center, t), t));
  }
}





void layout_painter::draw_layout(
  const wm::layout&   layout,
  float               alpha
) const {
  logging::debug("{}: rendering", config_.name);
  context_->make_current();

  glViewport(0, 0, geometry_.pixel_width(), geometry_.pixel_height());

  draw_wallpaper();
  draw_surface_effects(layout);
  draw_background(layout);

  if (alpha < 254.f / 255.f) {
    set_buffer_alpha(alpha);
  }
}



void layout_painter::draw_wallpaper() const {
  if (wallpaper_) {
    glClearColor(0.f, 0.f, 0.f, 0.f);
    glClear(GL_COLOR_BUFFER_BIT);

    texture_shader_.use();

    wallpaper_->bind();
    quad_.draw();

  } else {
    invoke_append_color(glClearColor, config_.wallpaper.solid);
    glClear(GL_COLOR_BUFFER_BIT);
  }
}



void layout_painter::draw_surface_effects(const wm::layout& layout) const {
  for (const auto& be: config_.border_effects) {
    for (const auto& surface: layout) {
      if (be.condition.evaluate(surface)) {
        draw_border_effect(be, surface);
      }
    }
    for (const auto& surface: fixed_panels_) {
      if (be.condition.evaluate(surface)) {
        draw_border_effect(be, surface);
      }
    }
  }
}



void layout_painter::draw_background(const wm::layout& layout) const {
  glDisable(GL_BLEND);

  draw_stenciled([&]() {
    solid_color_shader_.use();

    for (const auto& surface: layout) {
      if (config_.background_condition.evaluate(surface)) {
        draw_rectangle(surface.rect());
      }
    }

    for (const auto& surface: fixed_panels_) {
      if (config_.background_condition.evaluate(surface)) {
        draw_rectangle(surface.rect());
      }
    }

  }, [&](){
    if (background_) {
      texture_shader_.use();

      background_->bind();
      quad_.draw();

    } else {
      solid_color_shader_.use();

      invoke_append_color(glUniform4f, config_.background.solid, solid_color_uniform_);

      glUniformMatrix4fv(0, 1, GL_FALSE, mat4_unity.data());
      quad_.draw();
    }
  });
}



void layout_painter::set_buffer_alpha(float alpha) const {
  solid_color_shader_.use();
  glUniformMatrix4fv(0, 1, GL_FALSE, mat4_unity.data());

  glEnable(GL_BLEND);
  glBlendColor(alpha, alpha, alpha, alpha);
  glBlendFunc(GL_CONSTANT_COLOR, GL_CONSTANT_COLOR);

  quad_.draw();
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
