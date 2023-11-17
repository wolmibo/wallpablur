#ifndef WALLPABLUR_WAYLAND_OUTPUT_HPP_INCLUDED
#define WALLPABLUR_WAYLAND_OUTPUT_HPP_INCLUDED

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
  public:
    output(const output&) = delete;
    output(output&&)      = delete;
    output& operator=(const output&) = delete;
    output& operator=(output&&)      = delete;

    ~output();

    output(wl_ptr<wl_output>, client&);



    [[nodiscard]] wl_output* wl_raw() {
      return output_.get();
    }



    [[nodiscard]] std::string_view name() const {
      return name_ ? *name_ : std::string_view{"<?>"};
    }

    [[nodiscard]] vec2<uint32_t> current_size() const {
      return current_size_;
    }



    void set_done_cb(std::move_only_function<void(void)> fnc) {
      done_cb_ = std::move(fnc);
    }

    void set_size_cb(std::move_only_function<void(vec2<uint32_t>)> fnc) {
      size_cb_ = std::move(fnc);
    }



    [[nodiscard]] std::unique_ptr<surface> create_surface(std::string, bool);





  private:
    client*                                        client_;
    wl_ptr<wl_output>                              output_;

    vec2<uint32_t>                                 current_size_{};
    std::optional<std::string>                     name_;

    std::move_only_function<void(void)>            done_cb_;
    std::move_only_function<void(vec2<uint32_t>)>  size_cb_;



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
