#ifndef WALLPABLUR_RECTANGLE_HPP_INCLUDED
#define WALLPABLUR_RECTANGLE_HPP_INCLUDED

#include <array>
#include <limits>



class rectangle {
  public:
    rectangle() = default;

    rectangle(float x, float y, float width, float height, unsigned int rot_cw90 = 0) :
      x_       {x},
      y_       {y},
      width_   {std::max(width,  0.f)},
      height_  {std::max(height, 0.f)},

      rot_cw90_{rot_cw90 % 4}
    {}



    bool operator==(const rectangle&) const = default;





    [[nodiscard]] bool empty() const {
      return width_ * height_ < std::numeric_limits<float>::epsilon();
    }



    [[nodiscard]] float        x()        const { return x_;      }
    [[nodiscard]] float        y()        const { return y_;      }
    [[nodiscard]] float        width()    const { return width_;  }
    [[nodiscard]] float        height()   const { return height_; }
    [[nodiscard]] unsigned int rot_cw90() const { return rot_cw90_; }

    [[nodiscard]] std::array<float, 16> to_matrix(float, float) const;



    void translate(float x, float y) {
      x_ += x;
      y_ += y;
    }


    void inset(float thickness) {
      x_ += thickness;
      y_ += thickness;
      saturate_dec(width_,  2 * thickness);
      saturate_dec(height_, 2 * thickness);
    }



  private:
    float x_;
    float y_;
    float width_;
    float height_;

    unsigned int rot_cw90_;



    static void saturate_dec(float& val, float sub) {
      if (val < sub) {
        val = 0;
      } else {
        val -= sub;
      }
    }
};


#endif // WALLPABLUR_RECTANGLE_HPP_INCLUDED
