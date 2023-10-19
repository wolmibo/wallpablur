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
    static constexpr anchor_type all() {
      return anchor_type{0xf};
    }

    anchor_type() = default;

    [[nodiscard]] bool left()   const { return value_.test(a_left);   }
    [[nodiscard]] bool right()  const { return value_.test(a_right);  }
    [[nodiscard]] bool top()    const { return value_.test(a_top);    }
    [[nodiscard]] bool bottom() const { return value_.test(a_bottom); }

    void left  (bool v) { value_.set(a_left,   v); }
    void right (bool v) { value_.set(a_right,  v); }
    void top   (bool v) { value_.set(a_top,    v); }
    void bottom(bool v) { value_.set(a_bottom, v); }



    [[nodiscard]] std::bitset<4> value() const { return value_; }



  private:
    static constexpr size_t a_top    {0};
    static constexpr size_t a_right  {1};
    static constexpr size_t a_bottom {2};
    static constexpr size_t a_left   {3};

    std::bitset<4> value_;

    constexpr anchor_type(std::bitset<4> value) : value_{value} {}
};





using color = std::array<float, 4>;

}

#endif // WALLPABLUR_CONFIG_TYPES_HPP_INCLUDED
