#ifndef WALLPABLUR_WORKSPACE_EXPRESSION_HPP_INCLUDED
#define WALLPABLUR_WORKSPACE_EXPRESSION_HPP_INCLUDED

#include "wallpablur/expression/boolean.hpp"
#include "wallpablur/expression/string-compare.hpp"
#include "wallpablur/workspace.hpp"

#include <optional>
#include <string>
#include <variant>

#include <iconfigp/value-parser.hpp>



class workspace_expression_condition {
  public:
    enum class string_var {
      name,
      output
    };

    using string_expr = std::pair<expression::string_compare, string_var>;



    [[nodiscard]] static std::optional<workspace_expression_condition>
      from_string(std::string_view);



    [[nodiscard]] bool evaluate(const workspace&) const;



  private:
    std::variant<
      string_expr
    > cond_;



    template<typename...Args>
    workspace_expression_condition(Args&&... args) :
      cond_{std::forward<Args>(args)...}
    {}
};



using workspace_expression = expression::boolean<workspace_expression_condition>;



template<> struct iconfigp::value_parser<workspace_expression> {
  static constexpr std::string_view name {"workspace expression"};

  static std::string_view                    format();
  static std::optional<workspace_expression> parse(std::string_view);
};

#endif // WALLPABLUR_WORKSPACE_EXPRESSION_HPP_INCLUDED
