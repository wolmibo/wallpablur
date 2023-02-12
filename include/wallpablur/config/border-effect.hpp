#ifndef WALLPABLUR_CONFIG_BORDER_EFFECT_HPP_INCLUDED
#define WALLPABLUR_CONFIG_BORDER_EFFECT_HPP_INCLUDED

#include "wallpablur/config/types.hpp"



namespace config {

enum class blend_mode {
  add,
  alpha,
  replace
};



enum class falloff {
  step,
  linear,
  sinusoidal
};



enum class border_position {
  outside,
  inside,
  centered
};





struct border_effect {
  struct offset_type {
    int x{0};
    int y{0};
  };

  margin_type     thickness;
  border_position position {border_position::outside};
  offset_type     offset;

  color           col      {};
  blend_mode      blend    {blend_mode::alpha};
  falloff         foff     {falloff::step};
};

}

#endif // WALLPABLUR_CONFIG_BORDER_EFFECT_HPP_INCLUDED
