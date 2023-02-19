#ifndef WALLPABLUR_SURFACE_EXPRESSION_HPP_INCLUDED
#define WALLPABLUR_SURFACE_EXPRESSION_HPP_INCLUDED

#include "wallpablur/expression/boolean.hpp"
#include "wallpablur/surface.hpp"

#include <string>
#include <variant>



class surface_expression_condition {
  public:
    enum class flag {
      focused,
      urgent,
      panel,
      floating,
      tiled
    };



    [[nodiscard]] bool evaluate(const surface& surf) const {
      if (std::holds_alternative<flag>(cond_)) {
        switch (std::get<flag>(cond_)) {
          case flag::focused:  return surf.focused();
          case flag::urgent:   return surf.urgent();

          case flag::panel:    return surf.type() == surface_type::panel;
          case flag::floating: return surf.type() == surface_type::floating;
          case flag::tiled:    return surf.type() == surface_type::tiled;
        }

      } else if (std::holds_alternative<app_id>(cond_)) {
        return std::get<app_id>(cond_).value == surf.app_id();
      }

      std::unreachable();
    }



  private:
    struct app_id { std::string value; };

    std::variant<flag, app_id> cond_ = flag::focused;
};



using surface_expression = expression::boolean<surface_expression_condition>;


#endif // WALLPABLUR_SURFACE_EXPRESSION_HPP_INCLUDED
