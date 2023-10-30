// Copyright (c) 2023 wolmibo
// SPDX-License-Identifier: MIT

#ifndef WALLPABLUR_OUTPUT_HPP_INCLUDED
#define WALLPABLUR_OUTPUT_HPP_INCLUDED

#include "wallpablur/layout-painter.hpp"
#include "wallpablur/wm/change-token.hpp"

#include <memory>

namespace wayland {
  class output;
  class surface;
}



class output {
  public:
    output(const output&) = delete;
    output(output&&) noexcept = delete;
    output& operator=(const output&) = delete;
    output& operator=(output&&) noexcept = delete;
    ~output();

    output(std::unique_ptr<wayland::output>);



  private:
    std::unique_ptr<wayland::output>  wl_output_;

    std::unique_ptr<wayland::surface> wallpaper_surface_;
    std::unique_ptr<wayland::surface> clipping_surface_;

    float                             last_wallpaper_alpha_{-1.f};

    change_token<workspace>           layout_token_;

    std::optional<layout_painter>     painter_;



    void create_surfaces();
    void setup_surfaces();
};

#endif // WALLPABLUR_OUTPUT_HPP_INCLUDED
