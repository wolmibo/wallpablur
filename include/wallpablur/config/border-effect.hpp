#ifndef WALLPABLUR_CONFIG_BORDER_EFFECT_HPP_INCLUDED
#define WALLPABLUR_CONFIG_BORDER_EFFECT_HPP_INCLUDED

#include "wallpablur/config/types.hpp"
#include "wallpablur/surface-expression.hpp"



namespace config {

enum class blend_mode {
  add,
  alpha,
  replace
};



enum class falloff {
  none,
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

  surface_expression condition;

  uint32_t           thickness{};
  border_position    position {border_position::outside};
  offset_type        offset;

  color              col      {};
  blend_mode         blend    {blend_mode::alpha};
  falloff            foff     {falloff::none};
  float              exponent {1.f};
};

}

#endif // WALLPABLUR_CONFIG_BORDER_EFFECT_HPP_INCLUDED
