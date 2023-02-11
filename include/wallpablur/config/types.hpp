#ifndef WALLPABLUR_CONFIG_TYPES_HPP_INCLUDED
#define WALLPABLUR_CONFIG_TYPES_HPP_INCLUDED

#include <array>

#include <cstdint>



namespace config {

struct margin_type {
  int32_t left  {0};
  int32_t right {0};
  int32_t top   {0};
  int32_t bottom{0};


  margin_type() = default;

  margin_type(int32_t uni) :
    left{uni}, right{uni}, top{uni}, bottom{uni}
  {}

  margin_type(int32_t l, int32_t r, int32_t t, int32_t b) :
    left{l}, right{r}, top{t}, bottom{b}
  {}
};



using color = std::array<float, 4>;

}

#endif // WALLPABLUR_CONFIG_TYPES_HPP_INCLUDED
