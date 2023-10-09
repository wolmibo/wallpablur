#include "wallpablur/config/border-effect.hpp"
#include "wallpablur/config/config.hpp"
#include "wallpablur/config/filter.hpp"
#include "wallpablur/config/output.hpp"
#include "wallpablur/config/panel.hpp"
#include "wallpablur/config/types.hpp"
#include "wallpablur/surface-expression.hpp"

#include <chrono>
#include <iostream>
#include <optional>
#include <stdexcept>

#include <cassert>

#include <unistd.h>

#include <iconfigp/color.hpp>
#include <iconfigp/exception.hpp>
#include <iconfigp/parser.hpp>
#include <iconfigp/path.hpp>
#include <iconfigp/section.hpp>
#include <iconfigp/value-parser.hpp>

#include <logcerr/log.hpp>

using namespace std::string_view_literals;




namespace {
  [[nodiscard]] std::vector<std::string_view> split(std::string_view input, char delim) {
    std::vector<std::string_view> output;
    while (!input.empty()) {
      output.push_back(input.substr(0, input.find(delim)));
      input.remove_prefix(std::min(input.size(), output.back().size() + 1));
    }
    return output;
  }

  template<typename T>
  [[nodiscard]] bool parse_list(std::span<std::string_view> input, std::span<T> output) {
    assert(input.size() == output.size());
    for (size_t i = 0; i < input.size(); ++i) {
      if (auto value = iconfigp::value_parser<T>::parse(input[i])) {
        output[i] = *value;
      } else {
        return false;
      }
    }
    return true;
  }
}


template<> struct iconfigp::case_insensitive_parse_lut<config::wrap_mode> {
  static constexpr std::string_view name {"wrap-mode"};
  static constexpr std::array<std::pair<std::string_view, config::wrap_mode>, 4> lut {
    std::make_pair("none",         config::wrap_mode::none),
    std::make_pair("stretch-edge", config::wrap_mode::stretch_edge),
    std::make_pair("tiled",        config::wrap_mode::tiled),
    std::make_pair("tiled-mirror", config::wrap_mode::tiled_mirror),
  };
};

template<> struct iconfigp::case_insensitive_parse_lut<config::scale_mode> {
  static constexpr std::string_view name {"scale-mode"};
  static constexpr std::array<std::pair<std::string_view, config::scale_mode>, 4> lut {
    std::make_pair("fit",      config::scale_mode::fit),
    std::make_pair("zoom",     config::scale_mode::zoom),
    std::make_pair("stretch",  config::scale_mode::stretch),
    std::make_pair("centered", config::scale_mode::centered),
  };
};

template<> struct iconfigp::case_insensitive_parse_lut<config::scale_filter> {
  static constexpr std::string_view name{"scale-filter"};
  static constexpr std::array<std::pair<std::string_view, config::scale_filter>, 2> lut {
    std::make_pair("linear",  config::scale_filter::linear),
    std::make_pair("nearest", config::scale_filter::nearest)
  };
};

template<> struct iconfigp::case_insensitive_parse_lut<config::blend_mode> {
  static constexpr std::string_view name{"blend-mode"};
  static constexpr std::array<std::pair<std::string_view, config::blend_mode>, 3> lut {
    std::make_pair("add",     config::blend_mode::add),
    std::make_pair("alpha",   config::blend_mode::alpha),
    std::make_pair("replace", config::blend_mode::replace)
  };
};

template<> struct iconfigp::case_insensitive_parse_lut<config::falloff> {
  static constexpr std::string_view name{"falloff"};
  static constexpr std::array<std::pair<std::string_view, config::falloff>, 3> lut {
    std::make_pair("none",       config::falloff::none),
    std::make_pair("linear",     config::falloff::linear),
    std::make_pair("sinusoidal", config::falloff::sinusoidal)
  };
};

template<> struct iconfigp::case_insensitive_parse_lut<config::border_position> {
  static constexpr std::string_view name{"border-position"};
  static constexpr std::array<std::pair<std::string_view, config::border_position>, 3>
  lut {
    std::make_pair("outside",  config::border_position::outside),
    std::make_pair("inside",   config::border_position::inside),
    std::make_pair("centered", config::border_position::centered)
  };
};



enum class surface_effect_e {
  border,
  shadow,
  glow
};

template<> struct iconfigp::case_insensitive_parse_lut<surface_effect_e> {
  static constexpr std::string_view name{"border-effect"};
  static constexpr std::array<std::pair<std::string_view, surface_effect_e>, 3>
  lut {
    std::make_pair("border", surface_effect_e::border),
    std::make_pair("shadow", surface_effect_e::shadow),
    std::make_pair("glow",   surface_effect_e::glow)
  };
};

static const config::border_effect surface_effect_e_border {
  .condition = true,
  .thickness = 2,
  .position  = config::border_position::outside,
  .offset    = config::border_effect::offset_type{},
  .col       = {0.f, 0.f, 0.f, 1.f},
  .blend     = config::blend_mode::alpha,
  .foff      = config::falloff::none,
  .exponent  = 1.f
};

static const config::border_effect surface_effect_e_shadow {
  .condition = true,
  .thickness = 30,
  .position  = config::border_position::centered,
  .offset    = config::border_effect::offset_type {.x = 2, .y = 2},
  .col       = {0.f, 0.f, 0.f, 0.8f},
  .blend     = config::blend_mode::alpha,
  .foff      = config::falloff::sinusoidal,
  .exponent  = 1.5f
};

static const config::border_effect surface_effect_e_glow {
  .condition = true,
  .thickness = 20,
  .position  = config::border_position::outside,
  .offset    = config::border_effect::offset_type{},
  .col       = {1.f, 1.f, 1.f, 1.f},
  .blend     = config::blend_mode::add,
  .foff      = config::falloff::linear,
  .exponent  = 3.f
};

namespace {
  [[nodiscard]] const config::border_effect& surface_effect_e_default(
      surface_effect_e var
  ) {
    switch (var) {
      case surface_effect_e::border: return surface_effect_e_border;
      case surface_effect_e::shadow: return surface_effect_e_shadow;
      case surface_effect_e::glow:   return surface_effect_e_glow;
    }
    return surface_effect_e_border;
  }
}



enum class filter_e {
  invert,
  blur,
  box_blur
};

template<> struct iconfigp::case_insensitive_parse_lut<filter_e> {
  static constexpr std::string_view name {"filter-type"};
  static constexpr std::array<std::pair<std::string_view, filter_e>, 3> lut {
    std::make_pair("invert",   filter_e::invert),
    std::make_pair("box-blur", filter_e::box_blur),
    std::make_pair("blur",     filter_e::blur)
  };
};



template<> struct iconfigp::value_parser<config::margin_type> {
  static constexpr std::string_view name {"margin"};
  static constexpr std::string_view format() {
    return "<all:i32> or <left:i32>:<right:i32>:<top:i32>:<bottom:i32>";
  }
  static std::optional<config::margin_type> parse(std::string_view input) {
    if (auto value = iconfigp::value_parser<int32_t>::parse(input)) {
      return config::margin_type{*value};
    }
    auto list = split(input, ':');
    std::array<int32_t, 4> intlist{};

    if (list.size() != 4 || !parse_list<int32_t>(list, intlist)) {
      return {};
    }

    return config::margin_type{intlist[0], intlist[1], intlist[2], intlist[3]};
  }
};



template<> struct iconfigp::value_parser<config::anchor_type> {
  static constexpr std::string_view name {"anchor"};
  static constexpr std::string_view format() {
    return "string made up of l, r, b and t";
  }
  static std::optional<config::anchor_type> parse(std::string_view input) {
    config::anchor_type anchor;
    for (auto character: input) {
      if      (character == 'l') { anchor.left(true);   }
      else if (character == 'r') { anchor.right(true);  }
      else if (character == 'b') { anchor.bottom(true); }
      else if (character == 't') { anchor.top(true);    }
      else {
        return {};
      }
    }
    return anchor;
  }
};



template<> struct iconfigp::value_parser<config::panel::size_type> {
  static constexpr std::string_view name {"size"};
  static constexpr std::string_view format() { return "<width:u32>:<height:u32>"; }

  static std::optional<config::panel::size_type> parse(std::string_view input) {
    auto list = split(input, ':');
    std::array<uint32_t, 2> intlist{};
    if (list.size() != 2 || !parse_list<uint32_t>(list, intlist)) {
      return {};
    }

    return config::panel::size_type {
      .width  = intlist[0],
      .height = intlist[1]
    };
  }
};



template<> struct iconfigp::value_parser<config::border_effect::offset_type> {
  static constexpr std::string_view name {"size"};
  static constexpr std::string_view format() { return "<x:i32>,<y:i32>"; }

  static std::optional<config::border_effect::offset_type> parse(std::string_view in) {
    auto list = split(in, ',');
    std::array<int32_t, 2> intlist{};
    if (list.size() != 2 || !parse_list<int32_t>(list, intlist)) {
      return {};
    }

    return config::border_effect::offset_type {
      .x = intlist[0],
      .y = intlist[1]
    };
  }
};





namespace {
  template<typename Tgt> struct source_type { using type = Tgt; };
  template<> struct source_type<std::string> { using type = std::string_view; };
  template<> struct source_type<std::chrono::milliseconds> { using type = uint32_t; };

  template<typename Tgt, typename SrcKv, typename ...Args>
  void update(const SrcKv& section, Tgt& value, std::string_view key, Args&& ...args) {
    if (auto kv = section.unique_key(key)) {
      value = static_cast<Tgt>(iconfigp::parse<typename source_type<Tgt>::type>(*kv));
    } else if constexpr(sizeof...(Args)) {
      update(section, value, std::forward<Args>(args)...);
    }
  }



  [[nodiscard]] config::border_effect parse_border_effect(
      const iconfigp::group&       group,
      const config::border_effect& defaults
  ) {
    auto output = defaults;
    update(group, output.condition, "enable-if");
    update(group, output.thickness, "thickness");
    update(group, output.position,  "position");
    update(group, output.offset,    "offset");
    update(group, output.col,       "color");
    update(group, output.blend,     "blend");
    update(group, output.foff,      "falloff");
    update(group, output.exponent,  "exponent");
    return output;
  }



  [[nodiscard]] std::vector<config::border_effect> parse_border_effects(
    iconfigp::opt_ref<const iconfigp::section> section
  ) {
    if (!section) {
      return {};
    }

    std::vector<config::border_effect> effects;
    for (const auto& grp: section->groups()) {
      if (grp.count_keys("type") > 0) {
        auto type = iconfigp::parse<surface_effect_e>(grp.require_unique_key("type"));
        effects.emplace_back(parse_border_effect(grp, surface_effect_e_default(type)));
      }
    }
    return effects;
  }





  [[nodiscard]] config::panel parse_panel(const iconfigp::group& group) {
    config::panel output;
    update(group, output.anchor,    "anchor");
    update(group, output.size,      "size");
    update(group, output.margin,    "margin");
    update(group, output.radius,    "border-radius");
    update(group, output.app_id,    "app-id");
    update(group, output.focused,   "focused");
    update(group, output.urgent,    "urgent");
    update(group, output.condition, "enable-if");
    return output;
  }



  [[nodiscard]] std::vector<config::panel> parse_panels(
      iconfigp::opt_ref<const iconfigp::section> pan
  ) {
    if (!pan) {
      return {};
    }

    std::vector<config::panel> panels;
    for (const auto& grp: pan->groups()) {
      if (grp.count_keys("anchor") + grp.count_keys("size")
          + grp.count_keys("margin") > 0) {
        panels.emplace_back(parse_panel(grp));
      }
    }

    return panels;
  }



  [[nodiscard]] config::filter parse_filter(const iconfigp::group& filter) {
    auto filter_type = iconfigp::parse<filter_e>(filter.require_unique_key("filter"));

    switch (filter_type) {
      case filter_e::blur:
      case filter_e::box_blur: {
        config::box_blur_filter output;
        if (filter_type == filter_e::blur) {
          output.iterations = 2;
        }
        update(filter, output.width,      "width",  "radius"sv);
        update(filter, output.height,     "height", "radius"sv);
        update(filter, output.iterations, "iterations");
        update(filter, output.dithering,  "dithering");

        return output;
      }
      case filter_e::invert:
        return config::invert_filter{};
    }
    throw std::runtime_error{"filter type is not implemented"};
  }



  [[nodiscard]] std::vector<config::filter> parse_filters(const iconfigp::section& sec) {
    std::vector<config::filter> filters;
    for (const auto& grp: sec.groups()) {
      if (grp.count_keys("filter") > 0) {
        filters.emplace_back(parse_filter(grp));
      }
    }
    return filters;
  }



  [[nodiscard]] config::image_distribution parse_image_distribution(
    const iconfigp::section& section
  ) {
    config::image_distribution output{};
    update(section, output.scale,  "scale");
    update(section, output.wrap_x, "wrap-x", "wrap"sv);
    update(section, output.wrap_y, "wrap-y", "wrap"sv);
    update(section, output.filter, "scale-filter");
    return output;
  }



  [[nodiscard]] config::brush parse_brush(
      iconfigp::opt_ref<const iconfigp::section> section,
      iconfigp::opt_ref<const config::brush>     background
  ) {
    if (!section) {
      if (background) {
        return *background;
      }
      return config::brush{};
    }

    config::brush output;

    if (auto val = section->unique_key("color")) {
      output.solid = iconfigp::parse<config::color>(*val);
    } else if (background) {
      output.solid = background->solid;
    }

    if (auto path = section->unique_key("path")) {
      if (path->value().empty()) {
        return output;
      }
      output.fgraph = config::filter_graph {
        .path         = iconfigp::parse<std::filesystem::path>(*path),
        .distribution = parse_image_distribution(*section),
        .filters      = parse_filters(*section)
      };
      return output;
    }

    if (!background || !background->fgraph) {
      return output;
    }

    output.fgraph = background->fgraph;

    auto append_filters = parse_filters(*section);
    for (const auto& fil: append_filters) {
      output.fgraph->filters.emplace_back(fil);
    }

    return output;
  }



  [[nodiscard]] config::output parse_output(
      const iconfigp::section& section,
      iconfigp::opt_ref<const iconfigp::section> fallback
  ) {
    auto wallpaper_section = section.subsection("wallpaper");
    if (!wallpaper_section && fallback) {
      wallpaper_section = fallback->subsection("wallpaper");
    }
    auto wallpaper = parse_brush(wallpaper_section, {});



    auto background_section = section.subsection("background");
    if (!background_section && fallback) {
      background_section = fallback->subsection("background");
    }
    auto background = parse_brush(background_section, wallpaper);

    surface_expression background_condition{true};
    if (background_section) {
      if (auto key = background_section->unique_key("enable-if")) {
        background_condition = iconfigp::parse<surface_expression>(*key);
      }
    }



    auto panel_section = section.subsection("panels");
    if (!panel_section && fallback) {
      panel_section = fallback->subsection("panels");
    }

    auto border_effects_section = section.subsection("surface-effects");
    if (!border_effects_section && fallback) {
      border_effects_section = fallback->subsection("surface-effects");
    }



    return config::output {
      .name                 = std::string{section.name()},
      .wallpaper            = std::move(wallpaper),
      .background           = std::move(background),
      .background_condition = std::move(background_condition),
      .fixed_panels         = parse_panels(panel_section),
      .border_effects       = parse_border_effects(border_effects_section)
    };
  }
}





config::config::config(std::string_view input) {
  using std::chrono::milliseconds;

  try {
    auto root = iconfigp::parser::parse(input);

    update(root, poll_rate_,     "poll-rate-ms");
    update(root, fade_in_,       "fade-in-ms");
    update(root, fade_out_,      "fade-out-ms");

    update(root, disable_i3ipc_, "disable-i3ipc");

    update(root, as_overlay_,    "as-overlay");
    update(root, opacity_,       "opacity");
    update(root, gl_samples_,    "gl-samples");



    default_output_ = parse_output(root, {});

    for (const auto& section: root.subsections()) {
      if (section.name().empty()              ||
          section.name() == "panels"          ||
          section.name() == "wallpaper"       ||
          section.name() == "background"      ||
          section.name() == "surface-effects"
      ) {
        continue;
      }

      outputs_.emplace_back(parse_output(section, root));
    }

    if (auto message =
        iconfigp::generate_unused_message(root, input, logcerr::is_colored())) {

      logcerr::warn("config file contains unused keys");
      logcerr::print_raw_sync(std::cout, *message);
    }
  } catch (const iconfigp::exception& ex) {
    logcerr::error(ex.what());
    logcerr::print_raw_sync(std::cout,
        iconfigp::format_exception(ex, input, logcerr::is_colored()));
    throw false;
  }
}
