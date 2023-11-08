#include "wallpablur/expression/parser.hpp"
#include "wallpablur/expression/string-compare.hpp"
#include "wallpablur/surface-expression.hpp"


bool surface_expression_condition::evaluate(const surface& surf) const {
  switch (cond_.index()) {
    case 0:
      return surf.has_flag(std::get<0>(cond_));

    case 1: {
      const auto& [expr, var] = std::get<1>(cond_);

      switch (var) {
        case string_var::app_id: return expr.evaluate(surf.app_id());
      }
      return false;
    }

    default:
      static_assert(std::variant_size_v<decltype(cond_)> == 2);
      std::unreachable();
  }
}





std::string_view iconfigp::value_parser<surface_expression>::format() {
  return
    "Boolean expression formed from the terms:\n"
    "  <boolean>, <surface_flag>\n"
    "  app_id == [*]<string>[*]\n"
    "which can be combined (in descending precendence) using:\n"
    "  (), ! (prefix), && (infix), and || (infix)\n";
}





template<>
struct iconfigp::case_insensitive_parse_lut<surface_flag> {
  static constexpr std::string_view name {"surface-flag"};
  static constexpr std::array<std::pair<std::string_view, surface_flag>, 7> lut
  {
    std::make_pair("focused",    surface_flag::focused),
    std::make_pair("urgent",     surface_flag::urgent),

    std::make_pair("panel",      surface_flag::panel),
    std::make_pair("floating",   surface_flag::floating),
    std::make_pair("tiled",      surface_flag::tiled),
    std::make_pair("decoration", surface_flag::decoration),

    std::make_pair("fullscreen", surface_flag::fullscreen),
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
  [[nodiscard]] std::optional<surface_expression_condition::string_expr> parse_str_expr(
      std::string_view input
  ) {
    auto res = expression::parse_string_compare(input);
    if (!res) {
      return {};
    }

    auto& [expr, var_str] = *res;

    auto var = iconfigp::value_parser<surface_expression_condition::string_var>
                       ::parse(var_str);
    if (!var) {
      return {};
    }

    return std::make_pair(std::move(expr), *var);
  }
}



std::optional<surface_expression_condition> surface_expression_condition::from_token(
    expression::token in
) {
  if (auto flg = iconfigp::value_parser<surface_flag>::parse(in.content())) {
    return surface_expression_condition{*flg};
  }

  if (auto str = parse_str_expr(in.content())) {
    return surface_expression_condition{std::move(*str)};
  }

  return {};
}




std::optional<surface_expression> iconfigp::value_parser<surface_expression>::parse(
    std::string_view input
) {
  return expression::parser<surface_expression_condition>::parse(input);
}
