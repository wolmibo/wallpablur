#ifndef WALLPABLUR_LAYOUT_PAINTER_HPP_INCLUDED
#define WALLPABLUR_LAYOUT_PAINTER_HPP_INCLUDED

#include "wallpablur/config/border-effect.hpp"
#include "wallpablur/config/output.hpp"
#include "wallpablur/egl/context.hpp"
#include "wallpablur/texture-provider.hpp"
#include "wallpablur/wm/layout-manager.hpp"
#include "wallpablur/wayland/output.hpp"

#include <memory>

#include <gl/plane.hpp>
#include <gl/program.hpp>



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
    void draw_layout(const wm::layout&, float) const;



  private:
    config::output                        config_;
    std::shared_ptr<egl::context>         context_;
    std::shared_ptr<texture_provider>     texture_provider_;
    gl::plane                             quad_;
    gl::program                           solid_color_shader_;
    GLint                                 solid_color_uniform_;
    gl::program                           texture_shader_;

    enum class shader {
      border_step,
      border_linear,
      border_sinusoidal,
    };
    mutable flat_map<shader, gl::program> shader_cache_;

    std::shared_ptr<gl::texture>          wallpaper_;
    std::shared_ptr<gl::texture>          background_;

    wayland::geometry                     geometry_;
    wm::layout                            fixed_panels_;



    void draw_rectangle(const rectangle&) const;
    void draw_border_element(float x, float y, const rectangle&) const;
    void draw_border_effect(const config::border_effect&, const surface&) const;

    void draw_wallpaper()                        const;
    void draw_surface_effects(const wm::layout&) const;
    void draw_background     (const wm::layout&) const;
    void set_buffer_alpha    (float alpha)       const;
};


#endif // WALLPABLUR_LAYOUT_PAINTER_HPP_INCLUDED
