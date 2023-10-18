#ifndef WALLPABLUR_LAYOUT_PAINTER_HPP_INCLUDED
#define WALLPABLUR_LAYOUT_PAINTER_HPP_INCLUDED

#include "wallpablur/config/border-effect.hpp"
#include "wallpablur/config/output.hpp"
#include "wallpablur/egl/context.hpp"
#include "wallpablur/texture-provider.hpp"

#include <memory>

#include <gl/program.hpp>
#include <gl/mesh.hpp>



class layout_painter {
  public:
    layout_painter(const layout_painter&) = delete;
    layout_painter(layout_painter&&)      = default;
    layout_painter& operator=(const layout_painter&) = delete;
    layout_painter& operator=(layout_painter&&)      = default;

    ~layout_painter();

    layout_painter(config::output&&, std::shared_ptr<egl::context>,
        std::shared_ptr<texture_provider>);



    [[nodiscard]] const egl::context* context() const { return context_.get(); };



    bool update_geometry(const wayland::geometry&);
    void draw_layout(const workspace&, float) const;



  private:
    config::output                        config_;
    std::shared_ptr<egl::context>         context_;
    std::shared_ptr<texture_provider>     texture_provider_;
    gl::mesh                              quad_;
    gl::mesh                              sector_;
    gl::program                           solid_color_shader_;
    GLint                                 solid_color_uniform_;
    gl::program                           texture_shader_;

    enum class shader {
      border_step,
      border_linear,
      border_sinusoidal,
    };
    mutable flat_map<shader, gl::program> shader_cache_;

    wayland::geometry                     geometry_;
    std::vector<std::pair<surface, workspace_expression>>
                                          fixed_panels_;



    void draw_mesh(const rectangle&, const gl::mesh&) const;

    void draw_rounded_rectangle(const rectangle&, float) const;

    void draw_border_effect(const config::border_effect&, const surface&,
                             std::bitset<4> = 0xf) const;

    void draw_wallpaper      (const config::wallpaper&) const;
    void draw_surface_effects(const workspace&) const;
    void draw_background     (const config::background&, const workspace&) const;
    void set_buffer_alpha    (float alpha)      const;

    [[nodiscard]] const config::wallpaper* active_wallpaper(const workspace&) const;
};


#endif // WALLPABLUR_LAYOUT_PAINTER_HPP_INCLUDED
