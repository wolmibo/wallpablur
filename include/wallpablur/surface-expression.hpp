#ifndef WALLPABLUR_SURFACE_EXPRESSION_HPP_INCLUDED
#define WALLPABLUR_SURFACE_EXPRESSION_HPP_INCLUDED

#include "wallpablur/expression/boolean.hpp"
#include "wallpablur/surface.hpp"

#include <string>
#include <variant>

#include <iconfigp/value-parser.hpp>



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



    [[nodiscard]] static std::optional<surface_expression_condition>
      from_string(std::string_view);



    [[nodiscard]] bool evaluate(const surface& surf) const {
      if (std::holds_alternative<flag>(cond_)) {
        switch (std::get<flag>(cond_)) {
          case flag::focused:  return surf.focused();
          case flag::urgent:   return surf.urgent();

          case flag::panel:      return surf.type() == surface_type::panel;
          case flag::floating:   return surf.type() == surface_type::floating;
          case flag::tiled:      return surf.type() == surface_type::tiled;
          case flag::decoration: return surf.type() == surface_type::decoration;
        }

      } else if (std::holds_alternative<app_id>(cond_)) {
        return std::get<app_id>(cond_).value == surf.app_id();
      }

      std::unreachable();
    }



  private:
    struct app_id { std::string value; };

    std::variant<flag, app_id> cond_ = flag::focused;



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
