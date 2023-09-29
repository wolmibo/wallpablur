#include "wallpablur/expression/parser.hpp"
#include "wallpablur/expression/string-compare.hpp"
#include "wallpablur/surface-expression.hpp"
#include "wallpablur/workspace-expression.hpp"



bool workspace_expression_condition::evaluate(const workspace& ws) const {
  switch (cond_.index()) {
    case 0: {
      const auto& [expr, var] = std::get<string_expr>(cond_);

      switch (var) {
        case string_var::name:   return expr.evaluate(ws.name());
        case string_var::output: return expr.evaluate(ws.output());
      }

      return false;
    }
    case 1: {
      const auto& [expr, aggr] = std::get<1>(cond_);
      switch (aggr) {
        case bool_aggregator::all_of:
          return std::ranges::all_of(ws.surfaces(),
              [&expr](const surface& surf) { return expr.evaluate(surf); });
        case bool_aggregator::any_of:
          return std::ranges::any_of(ws.surfaces(),
              [&expr](const surface& surf) { return expr.evaluate(surf); });
        case bool_aggregator::none_of:
          return std::ranges::none_of(ws.surfaces(),
              [&expr](const surface& surf) { return expr.evaluate(surf); });
      }
    }
    default:
      break;
  }

  return false;
}





std::string_view iconfigp::value_parser<workspace_expression>::format() {
  return
    "Boolean expression formed from the terms:\n"
    "  (ws_name|output) == [*]<string>[*]\n"
    "which can be combined (in descending precendence) using:\n"
    "  (), ! (prefix), && (infix), and || (infix)\n";
}



template<>
struct iconfigp::case_insensitive_parse_lut<workspace_expression_condition::string_var> {
  static constexpr std::string_view name {"surface-variable"};
  static constexpr std::array<std::pair<std::string_view,
                                      workspace_expression_condition::string_var>, 3> lut
  {
    std::make_pair("ws_name", workspace_expression_condition::string_var::name),
    std::make_pair("ws-name", workspace_expression_condition::string_var::name),
    std::make_pair("output",  workspace_expression_condition::string_var::output),
  };
};




namespace {
  [[nodiscard]] std::optional<workspace_expression_condition::string_expr> parse_str_expr(
      std::string_view input
  ) {
    auto res = expression::parse_string_compare(input);
    if (!res) {
      return {};
    }

    auto& [expr, var_str] = *res;

    auto var = iconfigp::value_parser<workspace_expression_condition::string_var>
                       ::parse(var_str);
    if (!var) {
      return {};
    }

    return std::make_pair(std::move(expr), *var);
  }
}



std::optional<workspace_expression_condition> workspace_expression_condition::from_string(
    std::string_view in
) {
  if (auto str = parse_str_expr(in)) {
    return workspace_expression_condition{std::move(*str)};
  }

  return {};
}




std::optional<workspace_expression> iconfigp::value_parser<workspace_expression>::parse(
    std::string_view input
) {
  return expression::parser<workspace_expression_condition>::parse(input);
}

