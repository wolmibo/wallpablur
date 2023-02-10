#ifndef WALLPABLUR_WAYLAND_OUTPUT_HPP_INCLUDED
#define WALLPABLUR_WAYLAND_OUTPUT_HPP_INCLUDED

#include <wayland-client.h>
#include <wayland-egl.h>

#define namespace nmspace // NOLINT(*keyword-macro)
#include "wlr-layer-shell-unstable-v1-client-protocol.h"
#undef namespace
#include "viewporter-client-protocol.h"

#include "wallpablur/wayland/utils.hpp"
#include "wallpablur/wayland/geometry.hpp"

#include "wallpablur/egl/context.hpp"

#include <functional>
#include <memory>
#include <optional>
#include <string_view>



namespace wayland {

class client;

class output {
  public:
    output(const output&) = delete;
    output(output&&)      = delete;
    output& operator=(const output&) = delete;
    output& operator=(output&&)      = delete;

    ~output() = default;

    output(wl_ptr<wl_output>, client&);



    [[nodiscard]] bool                ready()   const { return ready_;        }

    [[nodiscard]] std::string_view    name()    const { return name_.value(); }
    [[nodiscard]] const egl::context& context() const { return *context_;     }

    [[nodiscard]] std::shared_ptr<egl::context> share_context() const {
      return context_;
    }



    void set_render_cb(std::move_only_function<void(geometry)>&& fnc) {
      render_cb_ = std::move(fnc);
    }

    void set_update_cb(std::move_only_function<bool(geometry)>&& fnc) {
      update_cb_ = std::move(fnc);
    }

    void set_ready_cb(std::move_only_function<void(output&)>&& fnc) {
      ready_cb_ = std::move(fnc);
      ready_    = false;
    }



  private:
    client*                                 client_;

    wl_ptr<wl_output>                       output_;
    wl_ptr<wl_surface>                      surface_;
    wl_ptr<wp_viewport>                     viewport_;
    wl_ptr<zwlr_layer_surface_v1>           layer_surface_;
    wl_ptr<wl_egl_window>                   egl_window_;
    std::shared_ptr<egl::context>           context_;
    wl_ptr<wl_callback>                     frame_callback_;



    bool                                    ready_;
    std::optional<std::string>              name_;

    geometry                                current_geometry_;

    std::move_only_function<void(geometry)> render_cb_;
    std::move_only_function<bool(geometry)> update_cb_;
    std::move_only_function<void(output&)>  ready_cb_;

    bool                                    first_configuration_{true};



    void render();
    void reset_frame_listener();

    void create_layer_surface();
    void create_context();

    void update_viewport() const;




    static void output_geometry_(
        void*       /*data*/,     wl_output* /*output*/,
        int32_t     /*x*/,        int32_t /*y*/,
        int32_t     /*phys_w*/,   int32_t /*phys_h*/,
        int32_t     /*subpixel*/,
        const char* /*make*/,     const char* /*model*/,
        int32_t     /*transform*/
    ) {}

    static void output_mode_(
        void*    /*data*/,    wl_output* /*output*/,
        uint32_t /*flags*/,
        int32_t  /*width*/,   int32_t /*height*/,
        int32_t  /*refresh*/
    ) {}

    static void output_description_(void* /*data*/, wl_output* /*output*/,
        const char* /*description*/) {}

    static void output_scale_(void* data, wl_output* /*output*/, int32_t scale) {
      static_cast<output*>(data)->current_geometry_.scale(scale);
    }

    static void output_name_ (void* data, wl_output* /*output*/, const char* name) {
      static_cast<output*>(data)->name_ = name;
    }

    static void output_done_ (void*, wl_output*);



    static constexpr wl_output_listener output_listener_ = {
      .geometry    = output_geometry_,
      .mode        = output_mode_,
      .done        = output_done_,
      .scale       = output_scale_,
      .name        = output_name_,
      .description = output_description_
    };





    static void layer_surface_closed_(void* /*data*/,
        zwlr_layer_surface_v1* /*surface*/) {}

    static void layer_surface_configure_(void*, zwlr_layer_surface_v1*,
        uint32_t, uint32_t, uint32_t);

    static constexpr zwlr_layer_surface_v1_listener layer_surface_listener_ = {
      .configure = layer_surface_configure_,
      .closed    = layer_surface_closed_
    };



    static void callback_done_(void*, wl_callback*, uint32_t);

    static constexpr wl_callback_listener callback_listener_ = {
      .done = callback_done_
    };
};

}

#endif // WALLPABLUR_WAYLAND_OUTPUT_HPP_INCLUDED
