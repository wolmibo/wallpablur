#include "wallpablur/config/border-effect.hpp"
#include "wallpablur/config/output.hpp"
#include "wallpablur/gl/utils.hpp"
#include "wallpablur/layout-painter.hpp"
#include "wallpablur/rectangle.hpp"
#include "shader/shader.hpp"

#include <algorithm>
#include <array>
#include <limits>

#include <logcerr/log.hpp>



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





  std::shared_ptr<egl::context> assert_non_nullptr(std::shared_ptr<egl::context> cx) {
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

  logcerr::verbose("{}: update geometry to logical = {}x{}@{}, pixel = {}x{}",
      config_.name,
      geometry_.logical_width(), geometry_.logical_height(), geometry_.scale(),
      geometry_.physical_width(), geometry_.physical_height());



  fixed_panels_.clear();
  fixed_panels_.reserve(config_.fixed_panels.size());

  for (const auto& panel : config_.fixed_panels) {
    fixed_panels_.emplace_back(surface{
        panel.to_rect(geometry_.logical_width(), geometry_.logical_height()),
        surface_type::panel,
        panel.app_id,
        panel.focused,
        panel.urgent,
        panel.radius
      },
      panel.condition
    );
  }



  for (auto& wp: config_.wallpapers) {
    if (wp.description.fgraph) {
      wp.description.realization = texture_provider_->get(geometry, wp.description);

      if (!wp.description.realization) {
        logcerr::warn("{}: retrieved empty wallpaper image", config_.name);
      }
    }

    if (wp.background.description.fgraph) {
      wp.background.description.realization
        = texture_provider_->get(geometry, wp.background.description);

      if (!wp.background.description.realization) {
        logcerr::warn("{}: retrieved empty background image", config_.name);
      }
    }
  }



  texture_provider_->cleanup();

  return true;
}





void layout_painter::draw_mesh(const rectangle& rect, const gl::mesh& mesh) const {
  glUniformMatrix4fv(0, 1, GL_FALSE,
      rect.to_matrix(geometry_.logical_width(), geometry_.logical_height()).data());

  mesh.draw();
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



  void set_border_uniforms(const config::border_effect& effect, float radius) {
    glUniform1f(20, effect.exponent);
    glUniform1f(21, 1.f + radius / effect.thickness);
    invoke_append_color(glUniform4f, effect.col, 10);
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





  [[nodiscard]] std::array<rectangle, 4> border_rectangles(rectangle r, float t) {
    auto&& w = r.width();
    auto&& h = r.height();

    return {
      rectangle{r.x(),         r.y() - t,     w, t, r.rot_cw90() + 0},
      rectangle{r.x() + w,     r.y(),         t, h, r.rot_cw90() + 1},
      rectangle{r.x(),         r.y() + h,     w, t, r.rot_cw90() + 2},
      rectangle{r.x() - t,     r.y(),         t, h, r.rot_cw90() + 3},
    };
  }

  [[nodiscard]] std::array<rectangle, 4> border_rectangles_in(rectangle r, float t) {
    auto&& w = r.width();
    auto&& h = r.height();

    return {
      rectangle{r.x(),         r.y(),         w, t, r.rot_cw90() + 2},
      rectangle{r.x() + w - t, r.y(),         t, h, r.rot_cw90() + 3},
      rectangle{r.x(),         r.y() + h - t, w, t, r.rot_cw90() + 0},
      rectangle{r.x(),         r.y(),         t, h, r.rot_cw90() + 1},
    };
  }



  [[nodiscard]] std::array<rectangle, 4> corner_rectangles(rectangle r, float t) {
    auto&& w = r.width();
    auto&& h = r.height();

    return {
      rectangle{r.x() + w, r.y() - t, t, t, r.rot_cw90() + 0},
      rectangle{r.x() + w, r.y() + h, t, t, r.rot_cw90() + 1},
      rectangle{r.x() - t, r.y() + h, t, t, r.rot_cw90() + 2},
      rectangle{r.x() - t, r.y() - t, t, t, r.rot_cw90() + 3}
    };
  }
}





void layout_painter::draw_rounded_rectangle(const rectangle& rect, float radius) const {
  radius = std::min({radius, rect.width(), rect.height()});

  if (radius < std::numeric_limits<float>::epsilon()) {
    draw_mesh(rect, quad_);
    return;
  }

  auto center = rect;
  center.inset(radius);

  draw_mesh(center, quad_);


  for (const auto& bord: border_rectangles(center, radius)) {
    draw_mesh(bord, quad_);
  }

  for (const auto& corn: corner_rectangles(center, radius)) {
    draw_mesh(corn, sector_);
  }
}





void layout_painter::draw_border_effect(
    const config::border_effect& effect,
    const surface&               surf,
    std::bitset<4>               sides
) const {
  if (sides.none()) {
    return;
  }

  set_blend_mode(effect.blend);

  auto center = center_tile(surf.rect(), effect);
  center.inset(surf.radius());

  if (sides.all()) {
    solid_color_shader_.use();
    invoke_append_color(glUniform4f, effect.col, solid_color_uniform_);
    draw_mesh(center, quad_);
  }


  switch (effect.foff) {
    case config::falloff::none:
      break;

    case config::falloff::linear:
      shader_cache_.find_or_create(shader::border_linear,
          resources::border_vs(), resources::border_linear_fs()).use();
      set_border_uniforms(effect, surf.radius());
      break;

    case config::falloff::sinusoidal:
      shader_cache_.find_or_create(shader::border_sinusoidal,
          resources::border_vs(), resources::border_sinusoidal_fs()).use();
      set_border_uniforms(effect, surf.radius());
      break;
  }



  if (float thickness = effect.thickness + surf.radius(); thickness > 0) {
    auto borders    = border_rectangles(center, thickness);
    auto borders_in = border_rectangles_in(center, thickness);

    static_assert(borders.size() == 4 && borders_in.size() == 4);
    for (size_t side = 0; side < 4; ++side) {
      if (!sides[side]) {
        continue;
      }

      //NOLINTNEXTLINE(*-constant-array-index)
      draw_mesh(borders[side], quad_);

      if (!sides.all()) {
        //NOLINTNEXTLINE(*-constant-array-index)
        draw_mesh(borders_in[side], quad_);
      }
    }

    size_t side{0};

    for (const auto& rect: corner_rectangles(center, thickness)) {
      if (sides[side] || sides[(side + 1) % sides.size()]) {
        draw_mesh(rect, sector_);
      }
      side++;
    }
  }
}





const config::wallpaper* layout_painter::active_wallpaper(const workspace& ws) const {
  for (const auto& wp: config_.wallpapers) {
    if (wp.condition.evaluate(ws)) {
      return &wp;
    }
  }

  return nullptr;
}





void layout_painter::draw_layout(const workspace& ws, float alpha) const {
  logcerr::debug("{}: rendering", config_.name);
  context_->make_current();

  glViewport(0, 0, geometry_.physical_width(), geometry_.physical_height());

  const auto* active = active_wallpaper(ws);

  if (active != nullptr) {
    draw_wallpaper(*active);
  }

  draw_surface_effects(ws);

  if (active != nullptr) {
    draw_background(active->background, ws);
  }

  if (alpha < 254.f / 255.f) {
    set_buffer_alpha(alpha);
  }
}



void layout_painter::draw_wallpaper(const config::wallpaper& wp) const {
  if (wp.description.realization) {
    glClearColor(0.f, 0.f, 0.f, 0.f);
    glClear(GL_COLOR_BUFFER_BIT);

    texture_shader_.use();

    wp.description.realization->bind();
    quad_.draw();
  } else {
    invoke_append_color(glClearColor, wp.description.solid);
    glClear(GL_COLOR_BUFFER_BIT);
  }
}



void layout_painter::draw_surface_effects(const workspace& ws) const {
  for (const auto& be: config_.border_effects) {
    for (const auto& surface: ws.surfaces()) {
      if (be.condition.evaluate(surface, ws)) {
        draw_border_effect(be, surface);
      }
    }
    for (const auto& [surface, condition]: fixed_panels_) {
      if (condition.evaluate(ws) && be.condition.evaluate(surface, ws)) {
        draw_border_effect(be, surface);
      }
    }
  }
}



void layout_painter::draw_background(
    const config::background& bg,
    const workspace&         ws
) const {
  glDisable(GL_BLEND);

  draw_stenciled([&]() {
    solid_color_shader_.use();

    for (const auto& surface: ws.surfaces()) {
      if (bg.condition.evaluate(surface, ws)) {
        draw_rounded_rectangle(surface.rect(), surface.radius());
      }
    }

    for (const auto& [surface, condition]: fixed_panels_) {
      if (condition.evaluate(ws) && bg.condition.evaluate(surface, ws)) {
        draw_rounded_rectangle(surface.rect(), surface.radius());
      }
    }

  }, [&](){
    if (bg.description.realization) {
      texture_shader_.use();

      bg.description.realization->bind();
      quad_.draw();

    } else {
      solid_color_shader_.use();

      invoke_append_color(glUniform4f, bg.description.solid, solid_color_uniform_);

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
    logcerr::error(ex.what());
  }
}
