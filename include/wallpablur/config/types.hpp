#ifndef WALLPABLUR_CONFIG_TYPES_HPP_INCLUDED
#define WALLPABLUR_CONFIG_TYPES_HPP_INCLUDED

#include <cstdint>



namespace config {

struct margin_type {
  int32_t left  {0};
  int32_t right {0};
  int32_t top   {0};
  int32_t bottom{0};
};

}

#endif // WALLPABLUR_CONFIG_TYPES_HPP_INCLUDED
