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





ICONFIGP_DEFINE_ENUM_LUT(surface_flag,
    "focused",    focused,
    "urgent",     urgent,
    "panel",      panel,
    "floating",   floating,
    "tiled",      tiled,
    "decoration", decoration,
    "fullscreen", fullscreen,

    "splitv",     splitv,
    "splith",     splith,
    "stacked",    stacked,
    "tabbed",     tabbed
);

ICONFIGP_DEFINE_ENUM_LUT_NAMED(surface_expression_condition::string_var,
    "surface-variable",
    "app_id", app_id, "app-id", app_id);





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
