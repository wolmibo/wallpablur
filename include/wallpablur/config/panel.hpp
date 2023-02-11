#ifndef WALLPABLUR_CONFIG_PANEL_HPP_INCLUDED
#define WALLPABLUR_CONFIG_PANEL_HPP_INCLUDED

#include "wallpablur/config/types.hpp"

#include <string>

#include <cstdint>



namespace config {

struct panel {
  struct anchor_type {
    static constexpr uint32_t left   {1 << 0};
    static constexpr uint32_t right  {1 << 1};
    static constexpr uint32_t top    {1 << 2};
    static constexpr uint32_t bottom {1 << 3};

    uint32_t value {0};
  };

  struct size_type {
    uint32_t width {0};
    uint32_t height{0};
  };



  anchor_type anchor;
  size_type   size;
  margin_type margin;


  std::string app_id;
  bool        focused{false};
  bool        urgent {false};
};

}

#endif // WALLPABLUR_CONFIG_PANEL_HPP_INCLUDED
