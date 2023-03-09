#include "wallpablur/expression/parser.hpp"
#include "wallpablur/surface-expression.hpp"

#include <iconfigp/value-parser.hpp>
#include <iconfigp/reader.hpp>



std::string_view iconfigp::value_parser<surface_expression>::format() {
  return
    "Boolean expression formed from the terms:\n"
    "  <boolean>, focused, urgent, panel, floating, tiled, decoration\n"
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



std::optional<surface_expression_condition> surface_expression_condition::from_string(
    std::string_view in
) {
  if (auto flg = iconfigp::value_parser<surface_expression_condition::flag>::parse(in)) {
    return surface_expression_condition{*flg};
  }

  return {};
}




std::optional<surface_expression> iconfigp::value_parser<surface_expression>::parse(
    std::string_view input
) {
  return expression::parser<surface_expression_condition>::parse(input);
}
