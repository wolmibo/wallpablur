#ifndef WALLPABLUR_CONFIG_OUTPUT_HPP_INCLUDED
#define WALLPABLUR_CONFIG_OUTPUT_HPP_INCLUDED

#include "wallpablur/config/border-effect.hpp"
#include "wallpablur/config/filter.hpp"
#include "wallpablur/config/panel.hpp"
#include "wallpablur/surface-workspace-expression.hpp"

#include <optional>
#include <string>
#include <vector>



namespace config {

struct brush {
  color                       solid {0.f, 0.f, 0.f, 0.f};
  std::optional<filter_graph> fgraph;

  bool operator==(const brush&) const = default;
};



struct output {
  std::string                  name;

  brush                        wallpaper;
  brush                        background;
  surface_workspace_expression background_condition{true};

  std::vector<panel>           fixed_panels;

  std::vector<border_effect>   border_effects;
};

}

#endif // WALLPABLUR_CONFIG_OUTPUT_HPP_INCLUDED
