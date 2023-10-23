#ifndef WALLPABLUR_CONFIG_OUTPUT_HPP_INCLUDED
#define WALLPABLUR_CONFIG_OUTPUT_HPP_INCLUDED

#include "wallpablur/config/border-effect.hpp"
#include "wallpablur/config/filter.hpp"
#include "wallpablur/config/panel.hpp"
#include "wallpablur/surface-workspace-expression.hpp"
#include "wallpablur/workspace-expression.hpp"

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <gl/texture.hpp>



namespace config {

struct brush {
  color                        solid {0.f, 0.f, 0.f, 0.f};
  std::optional<filter_graph>  fgraph;

  std::shared_ptr<gl::texture> realization;



  bool operator==(const brush& rhs) const {
    return solid == rhs.solid && fgraph == rhs.fgraph;
  };
};



struct background {
  brush                        description;
  surface_workspace_expression condition{true};
};



struct wallpaper {
  brush                        description;
  workspace_expression         condition{true};

  config::background           background;
};



struct surface_rounded_corners {
  surface_workspace_expression condition{true};
  float                        radius   {10.f};
};



struct output {
  std::string                          name;

  std::vector<wallpaper>               wallpapers;

  std::vector<panel>                   fixed_panels;

  std::vector<border_effect>           border_effects;
  std::vector<surface_rounded_corners> rounded_corners;
};

}

#endif // WALLPABLUR_CONFIG_OUTPUT_HPP_INCLUDED
