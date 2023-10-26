#ifndef WALLPABLUR_WAYLAND_OUTPUT_HPP_INCLUDED
#define WALLPABLUR_WAYLAND_OUTPUT_HPP_INCLUDED

#include <bitset>
#include <wayland-client.h>

#include "wallpablur/wayland/utils.hpp"
#include "wallpablur/wayland/geometry.hpp"

#include <functional>
#include <memory>
#include <optional>
#include <string_view>



namespace wayland {

class client;
class surface;

class output {
  friend class surface;

  public:
    output(const output&) = delete;
    output(output&&)      = delete;
    output& operator=(const output&) = delete;
    output& operator=(output&&)      = delete;

    ~output();

    output(wl_ptr<wl_output>, client&);




    [[nodiscard]] std::string_view name() const {
      return name_ ? *name_ : std::string_view{"<?>"};
    }



    [[nodiscard]] bool           ready()             const;

    [[nodiscard]] const surface& wallpaper_surface() const;
    [[nodiscard]] const surface& clipping_surface()  const;

    [[nodiscard]] bool has_clipping() const {
      return static_cast<bool>(clipping_surface_);
    }



    void set_render_cb(std::move_only_function<void(geometry)>&& fnc) {
      render_cb_ = std::move(fnc);
    }

    void set_update_cb(std::move_only_function<bool(geometry)>&& fnc) {
      update_cb_ = std::move(fnc);
    }

    void set_ready_cb(std::move_only_function<void(output&)>&& fnc) {
      ready_surfaces_.reset();
      ready_cb_ = std::move(fnc);
    }



  private:
    client*                                 client_;

    wl_ptr<wl_output>                       output_;

    geometry                                current_geometry_;
    std::optional<std::string>              name_;

    std::unique_ptr<surface>                wallpaper_surface_;
    std::unique_ptr<surface>                clipping_surface_;



    std::move_only_function<void(geometry)> render_cb_;
    std::move_only_function<bool(geometry)> update_cb_;
    std::move_only_function<void(output&)>  ready_cb_;

    std::bitset<2>                          ready_surfaces_;
    size_t                                  required_surfaces_;

    void mark_surface_ready(size_t);




    static void output_geometry_(
        void*       /*data*/,     wl_output*  /*output*/,
        int32_t     /*x*/,        int32_t     /*y*/,
        int32_t     /*phys_w*/,   int32_t     /*phys_h*/,
        int32_t     /*subpixel*/,
        const char* /*make*/,     const char* /*model*/,
        int32_t     /*transform*/
    ) {}

    static void output_description_(void* /*data*/, wl_output* /*output*/,
        const char* /*description*/) {}

    static void output_scale_(void* /*data*/, wl_output* /*output*/, int32_t /*scale*/){}

    static void output_mode_(void*, wl_output*, uint32_t, int32_t, int32_t, int32_t);
    static void output_name_(void*, wl_output*, const char*);
    static void output_done_(void*, wl_output*);



    static constexpr wl_output_listener output_listener_ = {
      .geometry    = output_geometry_,
      .mode        = output_mode_,
      .done        = output_done_,
      .scale       = output_scale_,
      .name        = output_name_,
      .description = output_description_
    };
};

}

#endif // WALLPABLUR_WAYLAND_OUTPUT_HPP_INCLUDED
