// Copyright (c) 2023 wolmibo
// SPDX-License-Identifier: MIT

#ifndef WALLPABLUR_WAYLAND_SURFACE_HPP_INCLUDED
#define WALLPABLUR_WAYLAND_SURFACE_HPP_INCLUDED

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



namespace wayland {

class client;
class output;

class surface {
  public:
    surface(const surface&) = delete;
    surface(surface&&)      = delete;
    surface& operator=(const surface&) = delete;
    surface& operator=(surface&&)      = delete;

    ~surface() = default;

    surface(std::string, client&, output&, bool);



    void hide();
    void show();

    [[nodiscard]] bool visible() const { return visible_; }



    void update_screen_size(vec2<uint32_t>);



    void set_update_cb(std::move_only_function<bool(void)> fnc) {
      update_cb_ = std::move(fnc);
    }

    void set_render_cb(std::move_only_function<void(void)> fnc) {
      render_cb_ = std::move(fnc);
    }

    void set_context_cb(std::move_only_function<void(std::shared_ptr<egl::context>)> f) {
      context_cb_ = std::move(f);
    }

    void set_geometry_cb(std::move_only_function<void(const geometry&)> fnc) {
      geometry_cb_ = std::move(fnc);
    }



  private:
    std::string                             name_;

    client*                                 client_;


    wl_ptr<wl_surface>                      surface_;
    wl_ptr<wp_viewport>                     viewport_;
    wl_ptr<zwlr_layer_surface_v1>           layer_surface_;
    wl_ptr<wl_egl_window>                   egl_window_;
    std::shared_ptr<egl::context>           context_;
    wl_ptr<wl_callback>                     frame_callback_;

    geometry                                current_geometry_;
    bool                                    invalid_            {false};

    bool                                    visible_            {true};
    bool                                    as_overlay_         {false};

    std::move_only_function<bool(void)>     update_cb_;
    std::move_only_function<void(void)>     render_cb_;
    std::move_only_function<void(std::shared_ptr<egl::context>)>
                                            context_cb_;
    std::move_only_function<void(const geometry&)>
                                            geometry_cb_;


    void invalidate() { invalid_ = true; }

    void render();
    void reset_frame_listener();

    bool update_context();

    void update_viewport() const;



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

#endif // WALLPABLUR_WAYLAND_SURFACE_HPP_INCLUDED
