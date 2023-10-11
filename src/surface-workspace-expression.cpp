#include "wallpablur/surface-workspace-expression.hpp"
#include "wallpablur/expression/parser.hpp"
#include "wallpablur/expression/tokenizer.hpp"
#include "wallpablur/surface-expression.hpp"
#include "wallpablur/workspace-expression.hpp"
#include "wallpablur/workspace.hpp"

#include <utility>



std::optional<surface_workspace_expression_condition>
surface_workspace_expression_condition::from_token(expression::token token) {
  if (auto surf_expr = surface_expression_condition::from_token(token)) {
    return surface_workspace_expression_condition{std::move(*surf_expr)};
  }

  if (auto ws_expr = workspace_expression_condition::from_token(token)) {
    return surface_workspace_expression_condition{std::move(*ws_expr)};
  }

  return {};
}



bool surface_workspace_expression_condition::evaluate(
    const surface&   surf,
    const workspace& ws
) const {
  switch (cond_.index()) {
    case 0: return std::get<0>(cond_).evaluate(surf);
    case 1: return std::get<1>(cond_).evaluate(ws);

    default:
      static_assert(std::variant_size_v<decltype(cond_)> == 2);
      std::unreachable();
  }
}





namespace {
  [[nodiscard]] constexpr std::string_view get_tokens(std::string_view input) {
    auto start = input.find("  ");
    return input.substr(start, input.find("which can be combined", start) - start);
  }
}



std::string iconfigp::value_parser<surface_workspace_expression>::format() {
  std::string output {"Boolean expression formed from surface token:\n"};

  output += get_tokens(iconfigp::value_parser<surface_expression>::format());
  output += "Or workspace token:\n";
  output += get_tokens(iconfigp::value_parser<workspace_expression>::format());
  output += "which can be combined (in descending precendence) using:\n"
            "  (), ! (prefix), && (infix), and || (infix)\n";

  return output;
}



std::optional<surface_workspace_expression>
iconfigp::value_parser<surface_workspace_expression>::parse(std::string_view input) {
  return expression::parser<surface_workspace_expression_condition>::parse(input);
}
