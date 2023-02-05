#ifndef WALLPABLUR_WAYLAND_CLIENT_HPP_INCLUDED
#define WALLPABLUR_WAYLAND_CLIENT_HPP_INCLUDED

#include <wayland-egl.h> // must be included before egl/context.hpp

#include "wallpablur/egl/context.hpp"
#include "wallpablur/flat-map.hpp"
#include "wallpablur/wayland/output.hpp"
#include "wallpablur/wayland/utils.hpp"

#include <functional>
#include <memory>
#include <string_view>



namespace wayland {

class client {
  public:
    client(const client&)     = delete;
    client(client&&) noexcept = delete;
    client& operator=(const client&)     = delete;
    client& operator=(client&&) noexcept = delete;

    ~client() = default;

    client();



    int  dispatch()  { return wl_display_dispatch(display_.get()); }
    void roundtrip() { wl_display_roundtrip(display_.get());       }

    void explore();



    [[nodiscard]] wl_compositor*       compositor()  const { return compositor_.get();  }
    [[nodiscard]] zwlr_layer_shell_v1* layer_shell() const { return layer_shell_.get(); }

    [[nodiscard]] egl::context&                 context()       { return *context_; }
    [[nodiscard]] std::shared_ptr<egl::context> share_context() { return context_;  }


    void set_output_add_cb(std::move_only_function<void(output&)>&& fnc) {
      output_add_callback_ = std::move(fnc);
    }

    void set_output_remove_cb(std::move_only_function<void(output&)>&& fnc) {
      output_remove_callback_ = std::move(fnc);
    }



  private:
    wl_ptr<wl_display>                          display_;
    std::shared_ptr<egl::context>               context_;
    wl_ptr<wl_registry>                         registry_;

    wl_ptr<wl_compositor>                       compositor_;
    wl_ptr<zwlr_layer_shell_v1>                 layer_shell_;

    flat_map<uint32_t, std::unique_ptr<output>> outputs_;


    std::move_only_function<void(output&)>      output_add_callback_;
    std::move_only_function<void(output&)>      output_remove_callback_;



    static void registry_global_(void*, wl_registry*, uint32_t, const char*, uint32_t);
    static void registry_global_remove_(void*, wl_registry*, uint32_t);

    static constexpr wl_registry_listener registry_listener_ = {
      .global        = registry_global_,
      .global_remove = registry_global_remove_
    };
};
}

#endif // WALLPABLUR_WAYLAND_CLIENT_HPP_INCLUDED
