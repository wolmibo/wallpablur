// Copyright (c) 2023 wolmibo
// SPDX-License-Identifier: MIT

#ifndef WALLPABLUR_SURFACE_WORKSPACE_EXPRESSION_HPP_INCLUDED
#define WALLPABLUR_SURFACE_WORKSPACE_EXPRESSION_HPP_INCLUDED

#include "wallpablur/expression/boolean.hpp"
#include "wallpablur/surface-expression.hpp"
#include "wallpablur/workspace-expression.hpp"

#include <optional>
#include <variant>

#include <iconfigp/value-parser.hpp>



namespace expression {
  class token;
}



class surface_workspace_expression_condition {
  public:
    [[nodiscard]] static std::optional<surface_workspace_expression_condition>
      from_token(expression::token);

    [[nodiscard]] bool evaluate(const surface&, const workspace&) const;



  private:
    std::variant<surface_expression_condition, workspace_expression_condition> cond_;

    template<typename...Args>
    surface_workspace_expression_condition(Args&&... args) :
      cond_{std::forward<Args>(args)...}
    {}
};



using surface_workspace_expression
  = expression::boolean<surface_workspace_expression_condition>;



template<> struct iconfigp::value_parser<surface_workspace_expression> {
  static constexpr std::string_view name {"surface workspace expression"};

  static std::string                                 format();
  static std::optional<surface_workspace_expression> parse(std::string_view);
};

#endif // WALLPABLUR_SURFACE_WORKSPACE_EXPRESSION_HPP_INCLUDED
