#ifndef WALLPABLUR_CONFIG_OUTPUT_HPP_INCLUDED
#define WALLPABLUR_CONFIG_OUTPUT_HPP_INCLUDED

#include "wallpablur/config/filter.hpp"
#include "wallpablur/config/panel.hpp"

#include <array>
#include <optional>
#include <string>
#include <vector>

namespace config {

using color = std::array<float, 4>;



struct brush {
  color                       solid {0.f, 0.f, 0.f, 0.f};
  std::optional<filter_graph> fgraph;

  bool operator==(const brush&) const = default;
};



struct output {
  std::string        name;

  brush              wallpaper;
  brush              background;

  std::vector<panel> fixed_panels;
};

}

#endif // WALLPABLUR_CONFIG_OUTPUT_HPP_INCLUDED
