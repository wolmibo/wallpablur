#ifndef WALLPABLUR_WAYLAND_CLIENT_HPP_INCLUDED
#define WALLPABLUR_WAYLAND_CLIENT_HPP_INCLUDED

#include <wayland-egl.h> // must be included before egl/context.hpp

#define namespace nmspace // NOLINT(*keyword-macro)
#include "wlr-layer-shell-unstable-v1-client-protocol.h"
#undef namespace
#include "viewporter-client-protocol.h"

#include "wallpablur/egl/context.hpp"
#include "wallpablur/wayland/utils.hpp"

#include <functional>
#include <memory>



namespace wayland {

class output;

class client {
  public:
    client(const client&)     = delete;
    client(client&&) noexcept = delete;
    client& operator=(const client&)     = delete;
    client& operator=(client&&) noexcept = delete;

    ~client() = default;

    client();



    void dispatch();
    void roundtrip();

    void explore();



    [[nodiscard]] wl_compositor*       compositor()  const { return compositor_.get();  }
    [[nodiscard]] zwlr_layer_shell_v1* layer_shell() const { return layer_shell_.get(); }
    [[nodiscard]] wp_viewporter*       viewporter()  const { return viewporter_.get();  }

    [[nodiscard]] egl::context&                 context()       { return *context_; }
    [[nodiscard]] std::shared_ptr<egl::context> share_context() { return context_;  }


    void set_output_add_cb(
        std::move_only_function<void(uint32_t, std::unique_ptr<output>)>&& fnc
    ) {
      output_add_callback_ = std::move(fnc);
    }

    void set_output_remove_cb(std::move_only_function<void(uint32_t)>&& fnc) {
      output_remove_callback_ = std::move(fnc);
    }



  private:
    wl_ptr<wl_display>                          display_;
    std::shared_ptr<egl::context>               context_;
    wl_ptr<wl_registry>                         registry_;

    wl_ptr<wl_compositor>                       compositor_;
    wl_ptr<zwlr_layer_shell_v1>                 layer_shell_;
    wl_ptr<wp_viewporter>                       viewporter_;


    std::move_only_function<void(uint32_t, std::unique_ptr<output>)>
                                                output_add_callback_;
    std::move_only_function<void(uint32_t)>     output_remove_callback_;



    static void registry_global_(void*, wl_registry*, uint32_t, const char*, uint32_t);
    static void registry_global_remove_(void*, wl_registry*, uint32_t);

    static constexpr wl_registry_listener registry_listener_ = {
      .global        = registry_global_,
      .global_remove = registry_global_remove_
    };
};
}

#endif // WALLPABLUR_WAYLAND_CLIENT_HPP_INCLUDED
