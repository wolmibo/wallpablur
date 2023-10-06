#ifndef WALLPABLUR_SURFACE_EXPRESSION_HPP_INCLUDED
#define WALLPABLUR_SURFACE_EXPRESSION_HPP_INCLUDED

#include "wallpablur/expression/boolean.hpp"
#include "wallpablur/expression/string-compare.hpp"
#include "wallpablur/surface.hpp"

#include <variant>

#include <iconfigp/value-parser.hpp>


namespace expression {
  class token;
}



class surface_expression_condition {
  public:
    enum class flag {
      focused,
      urgent,
      panel,
      floating,
      tiled,
      decoration,
    };

    enum class string_var {
      app_id,
    };

    using string_expr = std::pair<expression::string_compare, string_var>;



    [[nodiscard]] static std::optional<surface_expression_condition>
      from_token(expression::token);



    [[nodiscard]] bool evaluate(const surface& surf) const;



  private:
    std::variant<
      flag,
      string_expr
    > cond_ = flag::focused;



    template<typename... Args>
    surface_expression_condition(Args&&... args) :
      cond_{std::forward<Args>(args)...}
    {}
};



using surface_expression = expression::boolean<surface_expression_condition>;



template<> struct iconfigp::value_parser<surface_expression> {
  static constexpr std::string_view name {"surface expression"};

  static std::string_view                  format();
  static std::optional<surface_expression> parse(std::string_view);
};


#endif // WALLPABLUR_SURFACE_EXPRESSION_HPP_INCLUDED
