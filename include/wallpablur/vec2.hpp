// Copyright (c) 2023 wolmibo
// SPDX-License-Identifier: MIT

#ifndef WALLPABLUR_VEC2_HPP_INCLUDED
#define WALLPABLUR_VEC2_HPP_INCLUDED

#include <algorithm>
#include <array>
#include <cmath>



template<typename T>
class vec2 {
  public:
    vec2() = default;

    explicit vec2(const T& value) :
      x_{value},
      y_{value}
    {}

    explicit vec2(const std::array<T, 2>& values) :
      x_{values[0]},
      y_{values[1]}
    {}

    vec2(const T& x, const T& y) :
      x_{x},
      y_{y}
    {}

    bool operator==(const vec2<T>&) const = default;

    [[nodiscard]] T& x() { return x_; }
    [[nodiscard]] T& y() { return y_; }

    [[nodiscard]] const T& x() const { return x_; }
    [[nodiscard]] const T& y() const { return y_; }



    [[nodiscard]] vec2<T> operator+(const vec2<T>& rhs) const {
      return {x_ + rhs.x_, y_ + rhs.y_ };
    }

    [[nodiscard]] vec2<T> operator-(const vec2<T>& rhs) const {
      return {x_ - rhs.x_, y_ - rhs.y_ };
    }

    vec2<T>& operator+=(const vec2<T>& rhs) {
      x_ += rhs.x_; y_ += rhs.y_;
      return *this;
    }

    vec2<T>& operator-=(const vec2<T>& rhs) {
      x_ -= rhs.x_; y_ -= rhs.y_;
      return *this;
    }



    [[nodiscard]] vec2<T> operator*(const T& a) const {
      return {x_ * a, y_ * a};
    }

    [[nodiscard]] vec2<T> operator/(const T& a) const {
      return {x_ / a, y_ / a};
    }

    vec2<T>& operator*=(const T& a) {
      x_ *= a; y_ *= a;
      return *this;
    }

    vec2<T>& operator/=(const T& a) {
      x_ /= a; y_ /= a;
      return *this;
    }



    [[nodiscard]] friend vec2<T> operator*(const T& a, const vec2<T>& r) {
      return {a * r.x_, a * r.y_};
    }

    [[nodiscard]] friend vec2<T> operator/(const T& a, const vec2<T>& r) {
      return {a / r.x_, a / r.y_};
    }



  private:
    T x_;
    T y_;
};



template<typename To, typename From>
[[nodiscard]] constexpr vec2<To> vec_cast(const vec2<From>& from) {
  return {static_cast<To>(from.x()), static_cast<To>(from.y())};
}



template<typename T>
[[nodiscard]] constexpr vec2<T> max(const vec2<T>& l, const vec2<T>& r) {
  return {std::max<T>(l.x(), r.x()), std::max<T>(l.y(), r.y())};
}

template<typename T>
[[nodiscard]] constexpr vec2<T> min(const vec2<T>& l, const vec2<T>& r) {
  return {std::min<T>(l.x(), r.x()), std::min<T>(l.y(), r.y())};
}


template<typename T>
[[nodiscard]] constexpr vec2<T> mul(const vec2<T>& l, const vec2<T>& r) {
  return {l.x() * r.x(), l.y() * r.y()};
}

template<typename T>
[[nodiscard]] constexpr vec2<T> div(const vec2<T>& l, const vec2<T>& r) {
  return {l.x() / r.x(), l.y() / r.y()};
}


template<std::floating_point T>
[[nodiscard]] constexpr vec2<T> floor(const vec2<T>& v) {
  return {std::floor(v.x()), std::floor(v.y())};
}

#endif // WALLPABLUR_VEC2_HPP_INCLUDED
