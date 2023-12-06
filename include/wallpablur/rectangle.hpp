#ifndef WALLPABLUR_RECTANGLE_HPP_INCLUDED
#define WALLPABLUR_RECTANGLE_HPP_INCLUDED

#include "wallpablur/vec2.hpp"

#include <array>
#include <limits>



class rectangle {
  public:
    rectangle() = default;

    rectangle(vec2<float> pos, vec2<float> size, unsigned int rot_cw90 = 0) :
      pos_ {pos},
      size_{::max(size, vec2{0.f})},

      rot_cw90_{rot_cw90 % 4}
    {}



    bool operator==(const rectangle&) const = default;



    [[nodiscard]] bool empty() const {
      return size_.x() * size_.y() < std::numeric_limits<float>::epsilon();
    }


    [[nodiscard]] const vec2<float>& pos()  const { return pos_; }
    [[nodiscard]] vec2<float>&       pos()        { return pos_; }

    [[nodiscard]] const vec2<float>& size() const { return size_; }
    [[nodiscard]] vec2<float>&       size()       { return size_; }


    [[nodiscard]] vec2<float>        min()  const { return pos_;         }
    [[nodiscard]] vec2<float>        max()  const { return pos_ + size_; }


    [[nodiscard]] unsigned int rot_cw90() const { return rot_cw90_; }

    [[nodiscard]] std::array<float, 16> to_matrix(float, float) const;



    void inset(float thickness) {
      pos_  += vec2{thickness};
      size_ = ::max(size_ - vec2{2 * thickness}, vec2{0.f});
    }



  private:
    vec2<float> pos_;
    vec2<float> size_;

    unsigned int rot_cw90_;
};


#endif // WALLPABLUR_RECTANGLE_HPP_INCLUDED
