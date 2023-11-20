#include "wallpablur/application.hpp"
#include "wallpablur/config/border-effect.hpp"
#include "wallpablur/config/output.hpp"
#include "wallpablur/gl/utils.hpp"
#include "wallpablur/layout-painter.hpp"
#include "wallpablur/rectangle.hpp"
#include "wallpablur/surface.hpp"
#include "shader/shader.hpp"

#include <algorithm>
#include <array>
#include <limits>

#include <gl/framebuffer.hpp>

#include <logcerr/log.hpp>



namespace {
  using mat4 = std::array<GLfloat, 16>;



  constexpr mat4 mat4_unity {
    1.f, 0.f, 0.f, 0.f,
    0.f, 1.f, 0.f, 0.f,
    0.f, 0.f, 1.f, 0.f,
    0.f, 0.f, 0.f, 1.f
  };





  template<typename FNC, typename ...ARGS>
  void invoke_append_color(FNC&& f, const config::color& c, ARGS&&... args) {
    std::forward<FNC>(f)(std::forward<ARGS>(args)...,
        c[0] * c[3], c[1] * c[3], c[2] * c[3], c[3]);
  }





  void draw_mesh(const wayland::geometry& geo, const rectangle& r, const gl::mesh& mesh) {
    glUniformMatrix4fv(0, 1, GL_FALSE,
        r.to_matrix(geo.logical_width(), geo.logical_height()).data());

    mesh.draw();
  }





  [[nodiscard]] std::bitset<4> realize_sides(
      const config::sides_type& sides,
      layout_orientation        orientation
  ) {
    auto output = sides.absolute.value();
    auto rel    = sides.relative.value();

    if (output.all() || rel.none()) {
      return output;
    }

    switch (orientation) {
      case layout_orientation::horizontal:
        return output | (rel >> 1) | std::bitset<4>(rel[0] ? 0x8 : 0);
      case layout_orientation::vertical:
        return output | rel;

      default:
        return output;
    }
  }



  void set_blend_mode(config::blend_mode mode = config::blend_mode::alpha) {
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



  [[nodiscard]] std::array<rectangle, 8> corner_rectangles_alt(rectangle r, float t) {
    auto&& w = r.width();
    auto&& h = r.height();

    return {
      rectangle{r.x() - t,     r.y(),         t, t, r.rot_cw90() + 2},
      rectangle{r.x() + w - t, r.y() - t,     t, t, r.rot_cw90() + 3},
      rectangle{r.x() + w,     r.y() + h - t, t, t, r.rot_cw90() + 0},
      rectangle{r.x(),         r.y() + h,     t, t, r.rot_cw90() + 1},

      rectangle{r.x() + w,     r.y(),         t, t, r.rot_cw90() + 1},
      rectangle{r.x() + w - t, r.y() + h,     t, t, r.rot_cw90() + 2},
      rectangle{r.x() - t,     r.y() + h - t, t, t, r.rot_cw90() + 3},
      rectangle{r.x(),         r.y() - t,     t, t, r.rot_cw90() + 0}
    };
  }





  [[nodiscard]] const config::wallpaper* active_wallpaper(
      const workspace&                   ws,
      std::span<const config::wallpaper> wallpapers
  ) {
    for (const auto& wp: wallpapers) {
      if (wp.condition.evaluate(ws)) {
        return &wp;
      }
    }

    return nullptr;
  }





  [[nodiscard]] std::shared_ptr<egl::context> activate_context(
      std::shared_ptr<egl::context> ctx
  ) {
    if (!ctx) {
      throw std::runtime_error{"expected non-null context"};
    }
    ctx->make_current();
    return ctx;
  }
}





class layout_painter::radius_cache {
  public:
    explicit radius_cache(std::vector<config::surface_rounded_corners> conditions) :
      conditions_{std::move(conditions)}
    {}



    void update(
        const workspace&                                          ws,
        std::span<const std::pair<surface, workspace_expression>> fixed,
        uint64_t                                                  id
    ) {
      if (id == id_) {
        return;
      }

      id_ = id;
      cache_.clear();

      for (const auto& surf: ws.surfaces()) {
        if (auto r = radius(surf, ws); r > std::numeric_limits<float>::epsilon()) {
          cache_.emplace_back(&surf, radius(surf, ws));
        }
      }

      for (const auto& [surf, condition]: fixed) {
        if (!condition.evaluate(ws)) {
          continue;
        }

        if (auto r = radius(surf, ws); r > std::numeric_limits<float>::epsilon()) {
          cache_.emplace_back(&surf, radius(surf, ws));
        }
      }
    }



    [[nodiscard]] float radius(const surface& surface) const {
      if (auto it = std::ranges::find(cache_, &surface, &kv::first); it != cache_.end()) {
        return it->second;
      }
      return 0.f;
    }



    [[nodiscard]] bool empty() const { return cache_.empty(); }



  private:
    using kv = std::pair<const surface*, float>;

    std::vector<config::surface_rounded_corners> conditions_;
    std::vector<kv>                              cache_;
    uint64_t                                     id_{std::numeric_limits<uint64_t>::max()};



    [[nodiscard]] float radius(const surface& surf, const workspace& ws) const {
      float value = surf.radius();

      for (const auto& setting: conditions_) {
        if (setting.condition.evaluate(surf, ws)) {
          value = setting.radius;
        }
      }

      return value;
    }
};





struct layout_painter::wallpaper_context {
  std::shared_ptr<egl::context> context;
  gl::mesh                      quad;
  gl::mesh                      sector;
  gl::program                   solid_color_shader;
  GLint                         solid_color_color;
  gl::program                   solid_color_aa_shader;
  GLint                         solid_color_aa_color;
  gl::program                   texture_shader;
  GLint                         texture_alpha;
  gl::program                   texture_aa_shader;
  GLint                         texture_aa_alpha;
  GLint                         aa_cutoff;

  enum class shader {
    border_step,
    border_linear,
    border_sinusoidal
  };

  mutable flat_map<shader, gl::program> shader_cache;


  wallpaper_context(wallpaper_context&&) = delete;
  wallpaper_context(const wallpaper_context&) = delete;
  wallpaper_context& operator=(wallpaper_context&&) = delete;
  wallpaper_context& operator=(const wallpaper_context&) = delete;

  wallpaper_context(std::shared_ptr<egl::context> ctx) :
    context              {activate_context(std::move(ctx))},
    quad                 {gl::create_quad()},
    sector               {gl::create_sector(16)},
    solid_color_shader   {resources::solid_color_vs(), resources::solid_color_fs()},
    solid_color_color    {solid_color_shader.uniform("color_rgba")},
    solid_color_aa_shader{resources::solid_color_vs(),
                          resources::solid_color_aa_inner_fs()},
    solid_color_aa_color {solid_color_shader.uniform("color_rgba")},
    texture_shader       {resources::texture_vs(), resources::texture_fs()},
    texture_alpha        {texture_shader.uniform("alpha")},
    texture_aa_shader    {resources::texture_vs(), resources::texture_aa_inner_fs()},
    texture_aa_alpha     {texture_aa_shader.uniform("alpha")},
    aa_cutoff            {texture_aa_shader.uniform("cutoff")}
  {
    if (aa_cutoff != solid_color_aa_shader.uniform("cutoff")) {
      throw std::runtime_error{"uniform mismatch cutoff"};
    }
  }

  ~wallpaper_context() {
    context->make_current();
  }





  void draw_rounded_rectangle(
      const wayland::geometry& geo,
      const rectangle&         rect,
      float                    radius,
      const gl::program&       shader,
      const gl::program&       corner_shader
  ) const {
    radius = std::min({radius, rect.width(), rect.height()});

    shader.use();

    if (radius < std::numeric_limits<float>::epsilon()) {
      draw_mesh(geo, rect, quad);
      return;
    }

    auto center = rect;
    center.inset(radius);

    draw_mesh(geo, center, quad);

    for (const auto& bord: border_rectangles(center, radius)) {
      draw_mesh(geo, bord, quad);
    }

    corner_shader.use();
    glUniform1f(aa_cutoff, 0.75f / radius);

    for (const auto& corn: corner_rectangles(center, radius)) {
      draw_mesh(geo, corn, quad);
    }
  }





  void bind_effect_border(const config::border_effect& effect, float radius) const {
    switch (effect.foff) {
      case config::falloff::none:
        break;

      case config::falloff::linear:
        shader_cache.find_or_create(shader::border_linear,
            resources::border_vs(), resources::border_linear_fs()).use();
        set_border_uniforms(effect, radius);
        break;

      case config::falloff::sinusoidal:
        shader_cache.find_or_create(shader::border_sinusoidal,
            resources::border_vs(), resources::border_sinusoidal_fs()).use();
        set_border_uniforms(effect, radius);
        break;
    }
  }





  void draw_sides(
      const wayland::geometry& geo,
      std::bitset<4>           sides,
      rectangle                center,
      float                    thickness
  ) const {
    auto borders    = border_rectangles(center, thickness);
    auto borders_in = border_rectangles_in(center, thickness);

    constexpr auto S = sides.size();

    static_assert(borders.size() == S && borders_in.size() == S);
    for (size_t side = 0; side < S; ++side) {
      if (!sides[side]) {
        continue;
      }

      draw_mesh(geo, borders[side], quad);                //NOLINT(*-constant-array-index)

      if (!sides.all()) {
        draw_mesh(geo, borders_in[side], quad);           //NOLINT(*-constant-array-index)
      }
    }

    auto corners     = corner_rectangles(center, thickness);
    auto corners_alt = corner_rectangles_alt(center, thickness);

    static_assert(corners.size() == S && corners_alt.size() == S * 2);
    for (size_t side = 0; side < S; ++side) {
      if (!sides[(side + S - 1) % S] && sides[side]) {
        draw_mesh(geo, corners_alt[side], sector);        //NOLINT(*-constant-array-index)
      }

      if (sides[side] && !sides[(side + 1) % S]) {
        draw_mesh(geo, corners_alt[side + S], sector);    //NOLINT(*-constant-array-index)
      }

      if (sides[side] || sides[(side + 1) % S]) {
        draw_mesh(geo, corners[side], sector);            //NOLINT(*-constant-array-index)
      }
    }
  }





  void draw_border_effect(
    const wayland::geometry&     geo,
    const config::border_effect& effect,
    const surface&               surf,
    float                        radius
  ) const {
    auto sides = realize_sides(effect.sides, surf.orientation());

    if (sides.none()) {
      return;
    }

    set_blend_mode(effect.blend);

    auto center = center_tile(surf.rect(), effect);
    center.inset(radius);

    if (sides.all()) {
      solid_color_shader.use();
      invoke_append_color(glUniform4f, effect.col, solid_color_color);
      draw_mesh(geo, center, quad);
    }

    bind_effect_border(effect, radius);

    float thickness = effect.thickness + radius;
    if (thickness < std::numeric_limits<float>::epsilon()) {
      return;
    }

    draw_sides(geo, sides, center, thickness);

    set_blend_mode();
  }





  void draw_wallpaper(const config::wallpaper& wp) const {
    if (wp.description.realization) {
      glClearColor(0.f, 0.f, 0.f, 0.f);
      glClear(GL_COLOR_BUFFER_BIT);

      texture_shader.use();
      glUniform1f(texture_alpha, 1.f);
      glUniformMatrix4fv(0, 1, GL_FALSE, mat4_unity.data());

      wp.description.realization->bind();
      quad.draw();
    } else {
      invoke_append_color(glClearColor, wp.description.solid);
      glClear(GL_COLOR_BUFFER_BIT);
    }
  }





  void draw_surface_effects(
      const wayland::geometry&                                  geo,
      std::span<const config::border_effect>                    border_effects,
      const workspace&                                          ws,
      std::span<const std::pair<surface, workspace_expression>> fixed,
      const radius_cache&                                       radii
  ) const {
    for (const auto& be: border_effects) {
      for (const auto& surface: ws.surfaces()) {
        if (be.condition.evaluate(surface, ws)) {
          draw_border_effect(geo, be, surface, radii.radius(surface));
        }
      }
      for (const auto& [surface, condition]: fixed) {
        if (condition.evaluate(ws) && be.condition.evaluate(surface, ws)) {
          draw_border_effect(geo, be, surface, radii.radius(surface));
        }
      }
    }
  }





  [[nodiscard]] std::pair<const gl::program*, const gl::program*>
  setup_aa_shader(const config::background& bg) const {
    if (bg.description.realization) {
      texture_aa_shader.use();
      glUniform1f(texture_aa_alpha, 1.f);

      texture_shader.use();
      glUniform1f(texture_alpha, 1.f);

      bg.description.realization->bind();

      return {&texture_shader, &texture_aa_shader};
    }

    solid_color_aa_shader.use();
    invoke_append_color(glUniform4f, bg.description.solid, solid_color_aa_color);

    solid_color_shader.use();
    invoke_append_color(glUniform4f, bg.description.solid, solid_color_color);

    return {&solid_color_shader, &solid_color_aa_shader};
  }





  void draw_background(
      const wayland::geometry&                                  geo,
      const config::background&                                 bg,
      const workspace&                                          ws,
      std::span<const std::pair<surface, workspace_expression>> fixed,
      const radius_cache&                                       radii
  ) const {
    auto [shader, corner_shader] = setup_aa_shader(bg);

    for (const auto& surface: ws.surfaces()) {
      if (bg.condition.evaluate(surface, ws)) {
        draw_rounded_rectangle(geo, surface.rect(), radii.radius(surface),
            *shader, *corner_shader);
      }
    }

    for (const auto& [surface, condition]: fixed) {
      if (condition.evaluate(ws) && bg.condition.evaluate(surface, ws)) {
        draw_rounded_rectangle(geo, surface.rect(), radii.radius(surface),
            *shader, *corner_shader);
      }
    }
  }





  void set_buffer_alpha(float alpha) const {
    solid_color_shader.use();
    glUniform4f(solid_color_color, 0, 0, 0, 0);

    glUniformMatrix4fv(0, 1, GL_FALSE, mat4_unity.data());

    glEnable(GL_BLEND);
    glBlendColor(alpha, alpha, alpha, alpha);
    glBlendFunc(GL_CONSTANT_COLOR, GL_CONSTANT_COLOR);

    quad.draw();

    set_blend_mode();
  }
};





struct layout_painter::clipping_context {
  std::shared_ptr<egl::context> context;
  gl::mesh                      quad;
  gl::program                   texture_aa_shader;
  GLint                         texture_aa_shader_alpha;
  GLint                         texture_aa_shader_cutoff;

  gl::texture                   cached;
  wayland::geometry             cached_size;
  uint64_t                      cached_workspace_id{0};


  clipping_context(clipping_context&&) = delete;
  clipping_context(const clipping_context&) = delete;
  clipping_context& operator=(clipping_context&&) = delete;
  clipping_context& operator=(const clipping_context&) = delete;

  clipping_context(std::shared_ptr<egl::context> ctx) :
    context                 {activate_context(std::move(ctx))},
    quad                    {gl::create_quad()},
    texture_aa_shader       {resources::texture_vs(), resources::texture_aa_outer_fs()},
    texture_aa_shader_alpha {texture_aa_shader.uniform("alpha")},
    texture_aa_shader_cutoff{texture_aa_shader.uniform("cutoff")}
  {}

  ~clipping_context() {
    context->make_current();
  }





  void draw_corner_clipping(
      const wayland::geometry& geo,
      const rectangle&         rect,
      float                    radius
  ) const {
    radius = std::min({radius, rect.width(), rect.height()});

    if (radius < std::numeric_limits<float>::epsilon()) {
      return;
    }

    glUniform1f(texture_aa_shader_cutoff, 0.75f / radius);

    auto center = rect;
    center.inset(radius);

    for (const auto& corn: corner_rectangles(center, radius)) {
      draw_mesh(geo, corn, quad);
    }
  }
};





layout_painter::layout_painter(config::output config) :
  config_             {std::move(config)},
  texture_provider_   {app().texture_provider()},
  radius_cache_       {std::make_unique<radius_cache>(std::move(config_.rounded_corners))}
{}

layout_painter::layout_painter(layout_painter&&) noexcept = default;
layout_painter& layout_painter::operator=(layout_painter&&) noexcept = default;
layout_painter::~layout_painter() = default;





void layout_painter::set_wallpaper_context(std::shared_ptr<egl::context> context) {
  wallpaper_context_ = std::make_unique<wallpaper_context>(std::move(context));
}

void layout_painter::set_clipping_context(std::shared_ptr<egl::context> context) {
  clipping_context_ = std::make_unique<clipping_context>(std::move(context));
}





void layout_painter::update_geometry(const wayland::geometry& geometry) {
  if (geometry == geometry_) {
    return;
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
        panel.app_id,
        panel.mask,
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
}





void layout_painter::draw_wallpaper(const workspace& ws, uint64_t id) const {
  logcerr::debug("{}: drawing wallpaper", config_.name);

  glViewport(0, 0, geometry_.physical_width(), geometry_.physical_height());

  set_blend_mode();

  const auto* active = active_wallpaper(ws, config_.wallpapers);

  if (active != nullptr) {
    wallpaper_context_->draw_wallpaper(*active);
  }

  radius_cache_->update(ws, fixed_panels_, id);

  wallpaper_context_->draw_surface_effects(geometry_, config_.border_effects, ws,
      fixed_panels_, *radius_cache_);

  if (active != nullptr) {
    wallpaper_context_->draw_background(geometry_, active->background, ws,
        fixed_panels_, *radius_cache_);
  }
}





void layout_painter::update_cache(const workspace& ws, uint64_t id) const {
  if (!clipping_context_) {
    logcerr::warn("{}: trying to update cache without clipping context", config_.name);
    return;
  }

  if (!wallpaper_context_) {
    logcerr::warn("{}: trying to update cache without wallpaper context", config_.name);
    return;
  }

  bool requires_update{false};

  if (clipping_context_->cached_workspace_id != id) {
    requires_update = true;
    clipping_context_->cached_workspace_id = id;
  }

  wallpaper_context_->context->make_current();

  if (!clipping_context_->cached || clipping_context_->cached_size != geometry_) {
    requires_update = true;
    clipping_context_->cached = gl::texture(geometry_.physical_width(),
                                            geometry_.physical_height());

    clipping_context_->cached_size = geometry_;
  }


  if (!requires_update) {
    return;
  }

  logcerr::debug("{}: update cache", config_.name);

  gl::framebuffer fb{clipping_context_->cached};
  auto lock = fb.bind();

  glViewport(0, 0, geometry_.physical_width(), geometry_.physical_height());

  draw_wallpaper(ws, id);
}





void layout_painter::render_wallpaper(const workspace& ws, float a, uint64_t id) const {
  logcerr::debug("{}: rendering wallpaper {}x{}, alpha = {}", config_.name,
      geometry_.physical_width(), geometry_.physical_height(), a);

  if (!wallpaper_context_) {
    logcerr::warn("{}: trying to render wallpaper without context", config_.name);
    return;
  }

  wallpaper_context_->context->make_current();

  set_blend_mode();

  if (clipping_context_) {
    update_cache(ws, id);

    glClearColor(0.f, 0.f, 0.f, 0.f);
    glClear(GL_COLOR_BUFFER_BIT);

    wallpaper_context_->texture_shader.use();
    glUniform1f(wallpaper_context_->texture_alpha, a);
    glUniformMatrix4fv(0, 1, GL_FALSE, mat4_unity.data());

    clipping_context_->cached.bind();
    wallpaper_context_->quad.draw();

  } else {
    draw_wallpaper(ws, id);

    if (a < 254.f / 255.f) {
      wallpaper_context_->set_buffer_alpha(a);
    }
  }
}





void layout_painter::render_clipping(const workspace& ws, float a, uint64_t id) const {
  logcerr::debug("{}: rendering clipping {}x{}, alpha = {}", config_.name,
      geometry_.physical_width(), geometry_.physical_height(), a);

  if (!clipping_context_) {
    logcerr::warn("{}: trying to render clipping without context", config_.name);
    return;
  }

  update_cache(ws, id);

  clipping_context_->context->make_current();

  glClearColor(0.f, 0.f, 0.f, 0.f);
  glClear(GL_COLOR_BUFFER_BIT);

  set_blend_mode();

  clipping_context_->texture_aa_shader.use();
  glUniform1f(clipping_context_->texture_aa_shader_alpha, a);

  clipping_context_->cached.bind();



  for (const auto& surface: ws.surfaces()) {
    clipping_context_->draw_corner_clipping(geometry_, surface.rect(),
        radius_cache_->radius(surface));
  }

  for (const auto& [surface, condition]: fixed_panels_) {
    if (condition.evaluate(ws)) {
      clipping_context_->draw_corner_clipping(geometry_, surface.rect(),
          radius_cache_->radius(surface));
    }
  }
}





bool layout_painter::update_rounded_corners(const workspace& ws, uint64_t id) {
  radius_cache_->update(ws, fixed_panels_, id);
  return !radius_cache_->empty();
}
