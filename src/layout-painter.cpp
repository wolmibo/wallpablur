#include "wallpablur/config/border-effect.hpp"
#include "wallpablur/gl/utils.hpp"
#include "wallpablur/layout-painter.hpp"
#include "wallpablur/rectangle.hpp"
#include "shader/shader.hpp"

#include <algorithm>
#include <array>
#include <limits>

#include <logging/log.hpp>



namespace {
  using mat4 = std::array<GLfloat, 16>;



  constexpr mat4 mat4_unity {
    1.f, 0.f, 0.f, 0.f,
    0.f, 1.f, 0.f, 0.f,
    0.f, 0.f, 1.f, 0.f,
    0.f, 0.f, 0.f, 1.f
  };





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
                         return gl::create_quad();
                       }(*context_)},
  sector_             {gl::create_sector(16)},
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
    fixed_panels_.emplace_back(
        panel.to_rect(geometry_.logical_width(), geometry_.logical_height()),
        surface_type::panel,
        panel.app_id,
        panel.focused,
        panel.urgent,
        panel.radius
    );
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
      rect.to_matrix(geometry_.logical_width(), geometry_.logical_height()).data());

  quad_.draw();
}



void layout_painter::draw_rounded_rectangle(const rectangle& rect, float radius) const {
  radius = std::min({radius, rect.width(), rect.height()});

  if (radius < std::numeric_limits<float>::epsilon()) {
    draw_rectangle(rect);
    return;
  }

  auto center = rect;
  center.inset(radius);

  draw_rectangle(center);

  for (const auto& bord: center.border_rectangles(radius)) {
    draw_border_element(bord);
  }

  for (const auto& corn: center.corner_rectangles(radius)) {
    draw_corner_element(corn);
  }
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
    rect.translate(effect.offset.x, effect.offset.y);

    switch (effect.position) {
      case config::border_position::inside:
        rect.inset(effect.thickness);
        break;
      case config::border_position::centered:
        rect.inset(effect.thickness / 2.f);
        break;
      case config::border_position::outside:
        break;
    }

    return rect;
  }
}



void layout_painter::draw_border_element(const rectangle& rect) const {
  glUniformMatrix4fv(0, 1, GL_FALSE,
      rect.to_matrix(geometry_.logical_width(), geometry_.logical_height()).data());

  quad_.draw();
}



void layout_painter::draw_corner_element(const rectangle& rect) const {
  glUniformMatrix4fv(0, 1, GL_FALSE,
      rect.to_matrix(geometry_.logical_width(), geometry_.logical_height()).data());

  sector_.draw();
}



void layout_painter::draw_border_effect(
    const config::border_effect& effect,
    const surface&               surf
) const {
  set_blend_mode(effect.blend);

  solid_color_shader_.use();
  invoke_append_color(glUniform4f, effect.col, solid_color_uniform_);

  auto center = center_tile(surf.rect(), effect);
  draw_rectangle(center);


  switch (effect.foff) {
    case config::falloff::step:
      break;

    case config::falloff::linear:
      shader_cache_.find_or_create(shader::border_linear,
          resources::border_vs(), resources::border_linear_fs()).use();
      glUniform1f(20, effect.exponent);
      invoke_append_color(glUniform4f, effect.col, 10);
      break;

    case config::falloff::sinusoidal:
      shader_cache_.find_or_create(shader::border_sinusoidal,
          resources::border_vs(), resources::border_sinusoidal_fs()).use();
      glUniform1f(20, effect.exponent);
      invoke_append_color(glUniform4f, effect.col, 10);
      break;
  }



  if (float t = effect.thickness; t > 0) {
    for (const auto& rect: center.border_rectangles(t)) {
      draw_border_element(rect);
    }

    for (const auto& rect: center.corner_rectangles(t)) {
      draw_corner_element(rect);
    }
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
        draw_rounded_rectangle(surface.rect(), surface.radius());
      }
    }

    for (const auto& surface: fixed_panels_) {
      if (config_.background_condition.evaluate(surface)) {
        draw_rounded_rectangle(surface.rect(), surface.radius());
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
  glUniform4f(solid_color_uniform_, 0, 0, 0, 0);

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
