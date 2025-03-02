#ifndef WALLPABLUR_LAYOUT_PAINTER_HPP_INCLUDED
#define WALLPABLUR_LAYOUT_PAINTER_HPP_INCLUDED

#include "wallpablur/config/output.hpp"
#include "wallpablur/egl/context.hpp"
#include "wallpablur/texture-provider.hpp"

#include <memory>

#include <gl/program.hpp>
#include <gl/mesh.hpp>



class layout_painter {
  public:
    layout_painter(const layout_painter&) = delete;
    layout_painter(layout_painter&&) noexcept;
    layout_painter& operator=(const layout_painter&) = delete;
    layout_painter& operator=(layout_painter&&) noexcept;

    ~layout_painter();

    explicit layout_painter(config::output);



    void set_wallpaper_context(std::shared_ptr<egl::context>);
    void set_clipping_context (std::shared_ptr<egl::context>);



    void update_geometry(const wayland::geometry&);

    void render_wallpaper(const workspace&, float, uint64_t) const;
    void render_clipping(const workspace&, float, uint64_t) const;
    static void render_clear();

    bool update_rounded_corners(const workspace&, uint64_t);




  private:
    config::output                        config_;
    std::shared_ptr<texture_provider>     texture_provider_;

    struct wallpaper_context;
    struct clipping_context;
    class  radius_cache;

    std::unique_ptr<wallpaper_context>    wallpaper_context_;
    std::unique_ptr<clipping_context>     clipping_context_;

    std::unique_ptr<radius_cache>         radius_cache_;

    wayland::geometry                     geometry_;


    void draw_wallpaper(const workspace&, uint64_t) const;
    void update_cache(const workspace&, uint64_t) const;
};


#endif // WALLPABLUR_LAYOUT_PAINTER_HPP_INCLUDED
