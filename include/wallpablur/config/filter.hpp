#ifndef WALLPABLUR_CONFIG_FILTER_HPP_INCLUDED
#define WALLPABLUR_CONFIG_FILTER_HPP_INCLUDED

#include <filesystem>
#include <variant>
#include <vector>

#include <epoxy/gl.h>



namespace config {

struct invert_filter {
  bool operator==(const invert_filter& /*unused*/) const { return true; }
};



struct box_blur_filter {
  unsigned int width;
  unsigned int height;
  unsigned int iterations;

  bool operator==(const box_blur_filter&) const = default;
};



using filter = std::variant<
  invert_filter,
  box_blur_filter
>;





enum class scale_mode {
  fit,
  zoom,
  stretch,
  centered
};



enum class wrap_mode : GLint {
  none         = GL_CLAMP_TO_BORDER,
  stretch_edge = GL_CLAMP_TO_EDGE,
  tiled        = GL_REPEAT,
  tiled_mirror = GL_MIRRORED_REPEAT
};



enum class scale_filter {
  nearest = GL_NEAREST,
  linear  = GL_LINEAR
};



struct image_distribution {
  scale_mode   scale {scale_mode::zoom};
  wrap_mode    wrap_x{wrap_mode::none};
  wrap_mode    wrap_y{wrap_mode::none};
  scale_filter filter{scale_filter::linear};

  bool operator==(const image_distribution&) const = default;
};





struct filter_graph {
  std::filesystem::path path;
  image_distribution    distribution;
  std::vector<filter>   filters;

  bool operator==(const filter_graph&) const = default;
};

}

#endif // WALLPABLUR_CONFIG_FILTER_HPP_INCLUDED
