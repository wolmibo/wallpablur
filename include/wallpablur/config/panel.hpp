#ifndef WALLPABLUR_CONFIG_PANEL_HPP_INCLUDED
#define WALLPABLUR_CONFIG_PANEL_HPP_INCLUDED

#include "wallpablur/config/types.hpp"

#include <string>

#include <cstdint>



namespace config {

struct panel {
  struct size_type {
    uint32_t width {0};
    uint32_t height{0};
  };



  anchor_type anchor;
  size_type   size;
  margin_type margin;

  float       radius;


  std::string app_id;
  bool        focused{false};
  bool        urgent {false};
};

}

#endif // WALLPABLUR_CONFIG_PANEL_HPP_INCLUDED
