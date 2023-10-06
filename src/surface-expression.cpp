#include "wallpablur/expression/parser.hpp"
#include "wallpablur/expression/string-compare.hpp"
#include "wallpablur/surface-expression.hpp"


bool surface_expression_condition::evaluate(const surface& surf) const {
  if (std::holds_alternative<flag>(cond_)) {
    switch (std::get<flag>(cond_)) {
      case flag::focused:    return surf.focused();
      case flag::urgent:     return surf.urgent();

      case flag::panel:      return surf.type() == surface_type::panel;
      case flag::floating:   return surf.type() == surface_type::floating;
      case flag::tiled:      return surf.type() == surface_type::tiled;
      case flag::decoration: return surf.type() == surface_type::decoration;
    }
    return false;
  }



  if (std::holds_alternative<string_expr>(cond_)) {
    const auto& [expr, var] = std::get<string_expr>(cond_);

    switch (var) {
      case string_var::app_id: return expr.evaluate(surf.app_id());
    }
    return false;
  }



  std::unreachable();
}





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
  if (auto flg = iconfigp::value_parser<flag>::parse(in.content())) {
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
