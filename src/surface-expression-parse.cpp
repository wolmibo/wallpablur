#include "wallpablur/expression/parser.hpp"
#include "wallpablur/expression/string-compare.hpp"
#include "wallpablur/surface-expression.hpp"

#include <iconfigp/value-parser.hpp>
#include <iconfigp/reader.hpp>
#include <iconfigp/space.hpp>



std::string_view iconfigp::value_parser<surface_expression>::format() {
  return
    "Boolean expression formed from the terms:\n"
    "  <boolean>, focused, urgent, panel, floating, tiled, decoration,\n"
    "  app_id == [*]<string>[*]\n"
    "which can be combined (in descending precendence) using:\n"
    "  (), ! (prefix), && (infix), and || (infix)\n";
}





template<>
struct iconfigp::case_insensitive_parse_lut<surface_expression_condition::flag> {
  static constexpr std::string_view name {"surface-flag"};
  static constexpr std::array<std::pair<std::string_view,
                                        surface_expression_condition::flag>, 6> lut
  {
    std::make_pair("focused",  surface_expression_condition::flag::focused),
    std::make_pair("urgent",   surface_expression_condition::flag::urgent),

    std::make_pair("panel",      surface_expression_condition::flag::panel),
    std::make_pair("floating",   surface_expression_condition::flag::floating),
    std::make_pair("tiled",      surface_expression_condition::flag::tiled),
    std::make_pair("decoration", surface_expression_condition::flag::decoration),
  };
};



template<>
struct iconfigp::case_insensitive_parse_lut<surface_expression_condition::string_var> {
  static constexpr std::string_view name {"surface-variable"};
  static constexpr std::array<std::pair<std::string_view,
                                        surface_expression_condition::string_var>, 2> lut
  {
    std::make_pair("app_id", surface_expression_condition::string_var::app_id),
    std::make_pair("app-id", surface_expression_condition::string_var::app_id),
  };
};




namespace {
  [[nodiscard]] bool is_infix(char c) {
    return iconfigp::is_space(c) || c == '=';
  }



  [[nodiscard]] std::array<std::string_view, 3> split_string_expr(
      std::string_view input
  ) {
    input = iconfigp::trim_front(input);

    const auto* var_name_end = std::find_if    (input.begin(), input.end(), is_infix);
    const auto* value_begin  = std::find_if_not(var_name_end,  input.end(), is_infix);

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



  [[nodiscard]] std::optional<surface_expression_condition::string_expr> parse_str_expr(
      std::string_view input
  ) {
    auto [var_str, ops_str, value_str] = split_string_expr(input);

    auto var = iconfigp::value_parser<surface_expression_condition::string_var>
                       ::parse(var_str);
    if (!var) {
      return {};
    }

    auto ops = comparison(ops_str, value_str);
    if (!ops) {
      return {};
    }

    auto value = parse_value(value_str);
    if (!value) {
      return {};
    }

    return std::make_pair(expression::string_compare{std::move(*value), *ops}, *var);
  }
}



std::optional<surface_expression_condition> surface_expression_condition::from_string(
    std::string_view in
) {
  if (auto flg = iconfigp::value_parser<flag>::parse(in)) {
    return surface_expression_condition{*flg};
  }

  if (auto str = parse_str_expr(in)) {
    return surface_expression_condition{std::move(*str)};
  }

  return {};
}




std::optional<surface_expression> iconfigp::value_parser<surface_expression>::parse(
    std::string_view input
) {
  return expression::parser<surface_expression_condition>::parse(input);
}
