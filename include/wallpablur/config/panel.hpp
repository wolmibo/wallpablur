#ifndef WALLPABLUR_CONFIG_PANEL_HPP_INCLUDED
#define WALLPABLUR_CONFIG_PANEL_HPP_INCLUDED

#include "wallpablur/config/types.hpp"
#include "wallpablur/rectangle.hpp"
#include "wallpablur/surface.hpp"
#include "wallpablur/workspace-expression.hpp"

#include <string>

#include <cstdint>



namespace config {

struct panel {
  anchor_type          anchor {};
  vec2<uint32_t>       size   {0};
  margin_type          margin {};

  float                radius {0.f};


  std::string          app_id {};

  surface_flag_mask    mask   {};


  workspace_expression condition{true};



  [[nodiscard]] rectangle to_rect(const vec2<uint32_t>&) const;
};

}

#endif // WALLPABLUR_CONFIG_PANEL_HPP_INCLUDED
