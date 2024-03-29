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
    std::optional<config::output>     config_;

    std::unique_ptr<wayland::surface> wallpaper_surface_;
    std::unique_ptr<wayland::surface> clipping_surface_;

    float                             last_wallpaper_alpha_{-1.f};
    float                             last_clipping_alpha_ {-1.f};

    change_token<workspace>           layout_token_;

    std::optional<layout_painter>     painter_;


    std::bitset<2>                    surface_updated_;
    uint64_t                          last_layout_id_ {0};
    workspace                         last_layout_;

    std::vector<std::pair<surface, workspace_expression>>
                                      fixed_panels_;
    wayland::geometry                 geometry_;


    void create_surfaces(bool);
    void setup_surfaces();

    void update(bool);

    void update_geometry(const wayland::geometry&);
};

#endif // WALLPABLUR_OUTPUT_HPP_INCLUDED
