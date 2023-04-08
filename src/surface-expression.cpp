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
