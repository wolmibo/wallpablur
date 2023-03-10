#ifndef WALLPABLUR_CONFIG_TYPES_HPP_INCLUDED
#define WALLPABLUR_CONFIG_TYPES_HPP_INCLUDED

#include <array>
#include <bitset>

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





class anchor_type {
  public:
    [[nodiscard]] bool left()   const { return value.test(a_left);   }
    [[nodiscard]] bool right()  const { return value.test(a_right);  }
    [[nodiscard]] bool top()    const { return value.test(a_top);    }
    [[nodiscard]] bool bottom() const { return value.test(a_bottom); }

    void left  (bool v) { value.set(a_left,   v); }
    void right (bool v) { value.set(a_right,  v); }
    void top   (bool v) { value.set(a_top,    v); }
    void bottom(bool v) { value.set(a_bottom, v); }



  private:
    static constexpr size_t a_left   {0};
    static constexpr size_t a_right  {1};
    static constexpr size_t a_top    {2};
    static constexpr size_t a_bottom {3};

    std::bitset<4> value;
};





using color = std::array<float, 4>;

}

#endif // WALLPABLUR_CONFIG_TYPES_HPP_INCLUDED
