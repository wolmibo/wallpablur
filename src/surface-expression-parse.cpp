#include "wallpablur/surface-expression.hpp"



std::string_view iconfigp::value_parser<surface_expression>::format() {
  return
    "Boolean expression formed from the terms:\n"
    "  <boolean>, focused, urgent, panel, floating, tiled\n"
    "which can be combined (in descending precendence) using:\n"
    "  (), ! (prefix), && (infix), and || (infix)\n";
}



std::optional<surface_expression> iconfigp::value_parser<surface_expression>::parse(
    std::string_view input
) {
  throw iconfigp::value_parse_exception::range_exception{
    "Surface expression parser not yet implemented", 0, input.size()};
}
