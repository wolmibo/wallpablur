#include "wallpablur/expression/string-compare.hpp"

#include <algorithm>
#include <iconfigp/reader.hpp>



namespace {
  [[nodiscard]] bool is_infix(char c) {
    return iconfigp::is_space(c) || c == '=';
  }



  [[nodiscard]] std::array<std::string_view, 3> split_string_expr(
      std::string_view input
  ) {
    input = iconfigp::trim_front(input);

    const auto* var_name_end = std::ranges::find_if(input, is_infix);
    const auto* value_begin  = std::find_if_not(var_name_end, input.end(), is_infix);

    return {
      std::string_view{input.begin(), var_name_end},
      iconfigp::trim(std::string_view{var_name_end, value_begin}),
      iconfigp::trim_back(std::string_view{value_begin, input.end()})
    };
  }


  using cmp_mode = expression::string_compare::mode;


  [[nodiscard]] cmp_mode cmp_mode_from_value(std::string_view value) {
    if (value.size() > 1 && value.front() == '*' && value.back() == '*') {
      return cmp_mode::contains;
    }

    if (value.starts_with('*')) {
      return cmp_mode::ends_with;
    }

    if (value.ends_with('*')) {
      return cmp_mode::starts_with;
    }

    return cmp_mode::equal;
  }



  [[nodiscard]] std::optional<cmp_mode> comparison(
      std::string_view ops,
      std::string_view value
  ) {
    if (ops == "==") {
      return cmp_mode_from_value(value);
    }

    return {};
  }



  [[nodiscard]] std::optional<std::string> parse_value(std::string_view in) {
    if (in.starts_with('*')) {
      in.remove_prefix(1);
    }

    if (in.ends_with('*')) {
      in.remove_suffix(1);
    }

    auto reader = iconfigp::reader{in};
    try {
      return reader.read_until_one_of("()&|!=*").take_string();
    } catch (...) {
      return {};
    }
  }
}



std::optional<std::pair<expression::string_compare, std::string_view>>
expression::parse_string_compare(std::string_view input) {
  auto [var_str, ops_str, value_str] = split_string_expr(input);

  auto ops = comparison(ops_str, value_str);
  if (!ops) {
    return {};
  }

  auto value = parse_value(value_str);
  if (!value) {
    return {};
  }

  return std::make_pair(string_compare{std::move(*value), *ops}, var_str);
}
