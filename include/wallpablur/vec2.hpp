// Copyright (c) 2023 wolmibo
// SPDX-License-Identifier: MIT

#ifndef WALLPABLUR_VEC2_HPP_INCLUDED
#define WALLPABLUR_VEC2_HPP_INCLUDED

#include <algorithm>
#include <array>
#include <cmath>
#include <format>



template<typename T>
class vec2 {
  public:
    constexpr vec2() = default;

    constexpr explicit vec2(const T& value) :
      x_{value},
      y_{value}
    {}

    constexpr explicit vec2(const std::array<T, 2>& values) :
      x_{values[0]},
      y_{values[1]}
    {}

    constexpr vec2(const T& x, const T& y) :
      x_{x},
      y_{y}
    {}

    constexpr bool operator==(const vec2<T>&) const = default;

    [[nodiscard]] constexpr T& x() { return x_; }
    [[nodiscard]] constexpr T& y() { return y_; }

    [[nodiscard]] constexpr const T& x() const { return x_; }
    [[nodiscard]] constexpr const T& y() const { return y_; }


    [[nodiscard]] constexpr vec2<T> operator-() const {
      return {-x_, -y_};
    }


    [[nodiscard]] constexpr vec2<T> operator+(const vec2<T>& rhs) const {
      return {x_ + rhs.x_, y_ + rhs.y_ };
    }

    [[nodiscard]] constexpr vec2<T> operator-(const vec2<T>& rhs) const {
      return {x_ - rhs.x_, y_ - rhs.y_ };
    }

    constexpr vec2<T>& operator+=(const vec2<T>& rhs) {
      x_ += rhs.x_; y_ += rhs.y_;
      return *this;
    }

    constexpr vec2<T>& operator-=(const vec2<T>& rhs) {
      x_ -= rhs.x_; y_ -= rhs.y_;
      return *this;
    }



    [[nodiscard]] constexpr vec2<T> operator*(const T& a) const {
      return {x_ * a, y_ * a};
    }

    [[nodiscard]] constexpr vec2<T> operator/(const T& a) const {
      return {x_ / a, y_ / a};
    }

    constexpr vec2<T>& operator*=(const T& a) {
      x_ *= a; y_ *= a;
      return *this;
    }

    constexpr vec2<T>& operator/=(const T& a) {
      x_ /= a; y_ /= a;
      return *this;
    }



    [[nodiscard]] constexpr friend vec2<T> operator*(const T& a, const vec2<T>& r) {
      return {a * r.x_, a * r.y_};
    }

    [[nodiscard]] constexpr friend vec2<T> operator/(const T& a, const vec2<T>& r) {
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



template<std::formattable<char> T>
struct std::formatter<vec2<T>, char> {
  bool as_size = false;

  template<class ParseContext>
  constexpr ParseContext::iterator parse(ParseContext& ctx) {
    auto it = ctx.begin();

    if (it == ctx.end()) {
      return it;
    }

    if (*it == '#') {
      as_size = true;
      ++it;
    }

    if (*it != '}') {
      throw std::format_error{"Invalid format args for vec2<T>."};
    }

    return it;
  }

  template<class FmtContext>
  FmtContext::iterator format(const vec2<T>& vec, FmtContext& ctx) const {
    if (as_size) {
      return std::ranges::copy(std::format("{}×{}", vec.x(), vec.y()), ctx.out()).out;
    }
    return std::ranges::copy(std::format("({}, {})", vec.x(), vec.y()), ctx.out()).out;
  }
};

#endif // WALLPABLUR_VEC2_HPP_INCLUDED
