#ifndef ICONFIGP_VALUE_PARSER_HPP_INCLUDED
#define ICONFIGP_VALUE_PARSER_HPP_INCLUDED

#include "iconfigp/exception.hpp"
#include "iconfigp/format.hpp"
#include "iconfigp/key-value.hpp"
#include "iconfigp/opt-ref.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <limits>
#include <optional>
#include <sstream>
#include <string_view>

#include <cctype>



namespace iconfigp {


template<typename T>
struct value_parser {};





template<typename T>
struct floating_point {};

template<>
struct floating_point<float>       { static constexpr std::string_view name{"f32"};  };
template<>
struct floating_point<double>      { static constexpr std::string_view name{"f64"};  };
template<>
struct floating_point<long double> { static constexpr std::string_view name{"f128"}; };



template<typename T> requires (!floating_point<T>::name.empty())
struct value_parser<T> {
  static constexpr std::string_view name{floating_point<T>::name};

  static std::string format() {
    return "[floating point number]";
  }



  static std::optional<T> parse(std::string_view input) {
    T output{};
    const auto *end = input.data() + input.size();
    if (std::from_chars(input.data(), end, output).ptr == end) {
      return output;
    }

    return {};
  }
};





template<typename T>
struct integer {};

template<>
struct integer<int8_t>   { static constexpr std::string_view name{"i8"};  };
template<>
struct integer<uint8_t>  { static constexpr std::string_view name{"u8"};  };
template<>
struct integer<int16_t>  { static constexpr std::string_view name{"i16"}; };
template<>
struct integer<uint16_t> { static constexpr std::string_view name{"u16"}; };
template<>
struct integer<int32_t>  { static constexpr std::string_view name{"i32"}; };
template<>
struct integer<uint32_t> { static constexpr std::string_view name{"u32"}; };
template<>
struct integer<int64_t>  { static constexpr std::string_view name{"i64"}; };
template<>
struct integer<uint64_t> { static constexpr std::string_view name{"u64"}; };



template<typename T> requires (!integer<T>::name.empty())
struct value_parser<T> {
  static constexpr std::string_view name{integer<T>::name};

  static std::string format() {
    return iconfigp::format("integer from {} to {}",
      std::numeric_limits<T>::min(), std::numeric_limits<T>::max());
  }



  static std::optional<T> parse(std::string_view input) {
    T output{};
    const auto *end = input.data() + input.size();
    if (auto res = std::from_chars(input.data(), end, output);
        res.ptr == end && res.ec != std::errc::result_out_of_range) {

      return output;
    }

    return {};
  }
};





template<typename T>
struct case_insensitive_parse_lut {};

template<>
struct case_insensitive_parse_lut<bool> {
  static constexpr std::string_view name{"bool"};

  static constexpr std::array<std::pair<std::string_view, bool>, 10> lut {
    std::make_pair("true",  true),
    std::make_pair("t",     true),
    std::make_pair("yes",   true),
    std::make_pair("y",     true),
    std::make_pair("1",     true),

    std::make_pair("false", false),
    std::make_pair("f",     false),
    std::make_pair("no",    false),
    std::make_pair("n",     false),
    std::make_pair("0",     false),
  };
};



template<typename T> requires (!case_insensitive_parse_lut<T>::name.empty())
struct value_parser<T> {
  static constexpr std::string_view name{case_insensitive_parse_lut<T>::name};

  static std::string format() {
    std::stringstream output;
    output << '(';

    bool first{true};
    for (const auto& [label, _]: case_insensitive_parse_lut<T>::lut) {
      if (!first) {
        output << '|';
      }
      output << label;

      first = false;
    }

    output << ") // case insensitive";
    return output.str();
  }



  static std::optional<T> parse(std::string_view input) {
    auto it = std::ranges::find_if(case_insensitive_parse_lut<T>::lut,
        [input](const auto& p) {
          return std::ranges::equal(input, std::get<0>(p),
              [](char l, char r) { return std::tolower(l) == r; });
    });

    if (it != std::ranges::end(case_insensitive_parse_lut<T>::lut)) {
      return std::get<1>(*it);
    }
    return {};
  }
};





template<typename T>
concept value_parser_defined = requires () { value_parser<T>::name; };



template<value_parser_defined T>
[[nodiscard]] T parse(const key_value& value) {
  try {
    if (auto result = value_parser<T>::parse(value.value())) {
      return *result;
    }
  } catch (...) {}

  throw value_parse_exception{value, std::string{value_parser<T>::name},
    std::string{value_parser<T>::format()}};
}



template<value_parser_defined T>
[[nodiscard]] std::optional<T> parse(opt_ref<const key_value> value) {
  if (value) {
    return parse<T>(*value);
  }
  return {};
}

}


#endif // ICONFIGP_VALUE_PARSER_HPP_INCLUDED
