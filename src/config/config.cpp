#include "wallpablur/config/config.hpp"

#include "wallpablur/config/border-effect.hpp"
#include "wallpablur/config/filter.hpp"
#include "wallpablur/config/output.hpp"
#include "wallpablur/config/panel.hpp"
#include "wallpablur/config/types.hpp"
#include "wallpablur/exception.hpp"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <optional>
#include <string_view>

#include <unistd.h>

#include <iconfigp/array.hpp>
#include <iconfigp/color.hpp>
#include <iconfigp/exception.hpp>
#include <iconfigp/opt-ref.hpp>
#include <iconfigp/parser.hpp>
#include <iconfigp/path.hpp>
#include <iconfigp/section.hpp>
#include <iconfigp/value-parser.hpp>

#include <logcerr/log.hpp>

using namespace std::string_view_literals;
using namespace iconfigp;
using namespace config;

using cfg = ::config::config;

using opt_sec = iconfigp::opt_ref<const iconfigp::section>;



ICONFIGP_DEFINE_ENUM_LUT(wrap_mode,
    "none",         none,
    "stretch-edge", stretch_edge,
    "tiled",        tiled,
    "tiled-mirror", tiled_mirror)

ICONFIGP_DEFINE_ENUM_LUT(scale_mode,
    "fit",          fit,
    "zoom",         zoom,
    "stretch",      stretch,
    "centered",     centered)

ICONFIGP_DEFINE_ENUM_LUT(scale_filter,
    "linear",       linear,
    "nearest",      nearest)

ICONFIGP_DEFINE_ENUM_LUT(blend_mode,
    "add",          add,
    "alpha",        alpha,
    "replace",      replace)

ICONFIGP_DEFINE_ENUM_LUT(falloff,
    "none",         none,
    "linear",       linear,
    "sinusoidal",   sinusoidal)

ICONFIGP_DEFINE_ENUM_LUT(border_position,
    "outside",      outside,
    "inside",       inside,
    "centered",     centered)





enum class surface_effect_type : size_t {
  border   = 0,
  shadow   = 1,
  glow     = 2,

  rounded_corners = 3
};



namespace {
  constexpr sides_type sides_type_all_sides {
    .relative = {},
    .absolute = anchor_type::all()
  };

  const std::array<border_effect, 3> border_effects_default {
    border_effect /*border*/ {
      .condition = true,
      .thickness = 2,
      .position  = border_position::outside,
      .offset    = vec2{0.f},
      .col       = {0.f, 0.f, 0.f, 1.f},
      .blend     = blend_mode::alpha,
      .foff      = falloff::none,
      .exponent  = 1.f,
      .sides     = sides_type_all_sides
    },

    border_effect /*shadow*/ {
      .condition = true,
      .thickness = 30,
      .position  = border_position::centered,
      .offset    = vec2{2.f},
      .col       = {0.f, 0.f, 0.f, 0.8f},
      .blend     = blend_mode::alpha,
      .foff      = falloff::sinusoidal,
      .exponent  = 1.5f,
      .sides     = sides_type_all_sides
    },

    border_effect /*glow*/ {
      .condition = true,
      .thickness = 20,
      .position  = border_position::outside,
      .offset    = vec2{0.f},
      .col       = {1.f, 1.f, 1.f, 1.f},
      .blend     = blend_mode::add,
      .foff      = falloff::linear,
      .exponent  = 3.f,
      .sides     = sides_type_all_sides
    }
  };
}



ICONFIGP_DEFINE_ENUM_LUT(surface_effect_type,
    "border",          border,
    "shadow",          shadow,
    "glow",            glow,
    "rounded-corners", rounded_corners)



enum class filter_type {
  invert,
  blur,
  box_blur
};

ICONFIGP_DEFINE_ENUM_LUT(filter_type,
    "invert",   invert,
    "box-blur", box_blur,
    "blur",     blur)



ICONFIGP_DEFINE_VALUE_PARSER_NAMED(margin_type, "margin",
    "<all:i32> or <left:i32>:<right:i32>:<top:i32>:<bottom:i32>",
    [](std::string_view input) {
      return std::bit_cast<margin_type>(parse_as_array<int32_t, 4>(input));
    })

ICONFIGP_DEFINE_VALUE_PARSER_NAMED(vec2<uint32_t>, "vec2<u32>",
    "<width:u32>:<height:u32>",
    [](std::string_view input) {
      return vec2(parse_as_array<uint32_t, 2>(input));
    })

ICONFIGP_DEFINE_VALUE_PARSER_NAMED(vec2<float>, "vec2<f32>",
    "<x:f32>,<y:f32>",
    [](std::string_view input) {
      return vec2(parse_as_array<float, 2>(input));
    })

ICONFIGP_DEFINE_VALUE_PARSER_NAMED(anchor_type, "anchor",
    "string made up of l, r, b, and t",
    [](std::string_view input) -> std::optional<anchor_type> {
      anchor_type anchor;
      for (auto character: input) {
        switch (character) {
          case 'l': case 'L': anchor.left(true);   break;
          case 'r': case 'R': anchor.right(true);  break;
          case 'b': case 'B': anchor.bottom(true); break;
          case 't': case 'T': anchor.top(true);    break;
          default: return {};
        }
      }
      return anchor;
    })

ICONFIGP_DEFINE_VALUE_PARSER_NAMED(sides_type, "sides",
    "string made up of l, r, b, t, n, e, s, and w",
    [](std::string_view input) -> std::optional<sides_type> {
      anchor_type relative;
      anchor_type absolute;
      for (auto character: input) {
        switch (character) {
          case 'w': case 'W': relative.left(true);   break;
          case 'e': case 'E': relative.right(true);  break;
          case 's': case 'S': relative.bottom(true); break;
          case 'n': case 'N': relative.top(true);    break;

          case 'l': case 'L': absolute.left(true);   break;
          case 'r': case 'R': absolute.right(true);  break;
          case 'b': case 'B': absolute.bottom(true); break;
          case 't': case 'T': absolute.top(true);    break;
          default: return {};
        }
      }
      return sides_type(relative, absolute);
    })



namespace {
  template<typename Tgt> struct source_type { using type = Tgt; };
  template<> struct source_type<std::string> { using type = std::string_view; };
  template<> struct source_type<std::chrono::milliseconds> { using type = uint32_t; };

  template<typename Tgt, typename SrcKv, typename ...Args>
  void update(const SrcKv& section, Tgt& value, std::string_view key, Args&& ...args) {
    if (auto kv = section.unique_key(key)) {
      value = static_cast<Tgt>(parse<typename source_type<Tgt>::type>(*kv));
    } else if constexpr(sizeof...(Args)) {
      update(section, value, std::forward<Args>(args)...);
    }
  }





  [[nodiscard]] border_effect parse_border_effect(
      const group&         grp,
      const border_effect& defaults
  ) {
    auto output = defaults;
    update(grp, output.condition, "enable-if");
    update(grp, output.thickness, "thickness");
    update(grp, output.position,  "position");
    update(grp, output.offset,    "offset");
    update(grp, output.col,       "color");
    update(grp, output.blend,     "blend");
    update(grp, output.foff,      "falloff");
    update(grp, output.exponent,  "exponent");
    update(grp, output.sides,     "sides");
    return output;
  }



  [[nodiscard]] surface_rounded_corners parse_rounded_corners(const group& grp) {
    surface_rounded_corners output;
    update(grp, output.condition, "enable-if");
    update(grp, output.radius,    "radius");
    return output;
  }



  [[nodiscard]]
  std::pair<std::vector<surface_rounded_corners>, std::vector<border_effect>>
  parse_surface_effects(opt_sec sec) {
    if (!sec) {
      return {};
    }

    std::vector<surface_rounded_corners> rounded_corners;
    std::vector<border_effect>           border_effects;

    for (const auto& grp: sec->groups()) {
      if (auto type = grp.unique_key("type")) {
        auto tp = parse<surface_effect_type>(*type);

        switch (tp) {
          case surface_effect_type::rounded_corners:
            if (auto effect = parse_rounded_corners(grp);
                !effect.condition.is_always_false()) {
              rounded_corners.emplace_back(std::move(effect));
            }
            break;

          default:
            if (auto effect = parse_border_effect(grp,
                                border_effects_default.at(std::to_underlying(tp)));
                !effect.condition.is_always_false()) {
              border_effects.emplace_back(std::move(effect));
            }
            break;
        }
      }
    }


    return {std::move(rounded_corners), std::move(border_effects)};
  }





  [[nodiscard]] panel parse_panel(const group& grp) {
    panel output;

    set_flag(output.mask, surface_flag::panel);

    if (auto key = grp.unique_key("focused"); key && parse<bool>(*key)) {
      set_flag(output.mask, surface_flag::focused);
    }
    if (auto key = grp.unique_key("urgent"); key && parse<bool>(*key)) {
      set_flag(output.mask, surface_flag::urgent);
    }

    update(grp, output.anchor,    "anchor");
    update(grp, output.size,      "size");
    update(grp, output.margin,    "margin");
    update(grp, output.radius,    "radius");
    update(grp, output.app_id,    "app-id", "app_id"sv);
    update(grp, output.condition, "enable-if");

    return output;
  }



  [[nodiscard]] std::vector<panel> parse_panels(opt_ref<const section> pan) {
    if (!pan) {
      return {};
    }

    std::vector<panel> panels;
    for (const auto& grp: pan->groups()) {
      if (grp.count_keys("anchor") + grp.count_keys("size")
          + grp.count_keys("margin") > 0) {
        panels.emplace_back(parse_panel(grp));
      }
    }

    return panels;
  }



  [[nodiscard]] filter parse_filter(const group& filter) {
    auto type = parse<filter_type>(filter.require_unique_key("filter"));

    switch (type) {
      case filter_type::blur:
      case filter_type::box_blur: {
        box_blur_filter output;
        if (type == filter_type::blur) {
          output.iterations = 2;
        }
        update(filter, output.size.x(),   "width",  "radius"sv);
        update(filter, output.size.y(),   "height", "radius"sv);
        update(filter, output.iterations, "iterations");
        update(filter, output.dithering,  "dithering");

        return output;
      }
      case filter_type::invert:
        return invert_filter{};
    }
    throw ::exception{"filter type is not implemented"};
  }



  [[nodiscard]] std::vector<filter> parse_filters(const section& sec) {
    std::vector<filter> filters;
    for (const auto& grp: sec.groups()) {
      if (grp.count_keys("filter") > 0) {
        filters.emplace_back(parse_filter(grp));
      }
    }
    return filters;
  }



  [[nodiscard]] image_distribution parse_image_distribution(const section& sec) {
    image_distribution output{};
    update(sec, output.scale,  "scale");
    update(sec, output.wrap_x, "wrap-x", "wrap"sv);
    update(sec, output.wrap_y, "wrap-y", "wrap"sv);
    update(sec, output.filter, "scale-filter");
    return output;
  }



  [[nodiscard]] brush parse_brush(opt_sec sec, opt_ref<const brush> background) {
    if (!sec) {
      if (background) {
        return *background;
      }
      return brush{};
    }

    brush output{};

    if (auto val = sec->unique_key("color")) {
      output.solid = parse<color>(*val);
    } else if (background) {
      output.solid = background->solid;
    }

    if (auto path = sec->unique_key("path")) {
      if (path->value().empty()) {
        return output;
      }
      output.fgraph = filter_graph {
        .path         = parse<std::filesystem::path>(*path),
        .distribution = parse_image_distribution(*sec),
        .filters      = parse_filters(*sec)
      };
      return output;
    }

    if (!background || !background->fgraph) {
      return output;
    }

    output.fgraph = background->fgraph;

    auto append_filters = parse_filters(*sec);
    for (const auto& fil: append_filters) {
      output.fgraph->filters.emplace_back(fil);
    }

    return output;
  }



  [[nodiscard]] std::pair<const section*, opt_sec> select_section_sub(
      const section&   sec,
      opt_sec          fallback,
      std::string_view name
  ) {
    auto cand = sec.subsection(name);
    if (!cand && fallback) {
      return {&*fallback, fallback->subsection(name)};
    }
    return {&sec, cand};
  }



  [[nodiscard]] wallpaper parse_wallpaper(opt_sec sec, opt_sec bg_sec, bool alt) {
    wallpaper wp{};

    wp.description = parse_brush(sec, {});

    if (alt && sec) {
      update(*sec, wp.condition, "enable-if");
    }

    wp.background.description = parse_brush(bg_sec, wp.description);

    if (bg_sec) {
      update(*bg_sec, wp.background.condition, "enable-if");
    }

    return wp;
  }



  [[nodiscard]] std::vector<const section*> list_wallpaper_sections(const section& sec) {
    std::vector<std::pair<size_t, const section*>> list;
    for (const auto& s: sec.subsections()) {
      if (s.name().starts_with("wallpaper#")) {
        if (auto value = value_parser<size_t>::parse(s.name().substr(10))) {
          list.emplace_back(*value, &s);
        } else {
          logcerr::warn("found section {}, did you mean wallpaper#<int>?", s.name());
        }
      }
    }

    std::ranges::sort(list, {}, &std::pair<size_t, const section*>::first);

    std::vector<const section*> slice;
    slice.reserve(list.size());
    for (const auto& [_, ptr]: list) {
      slice.emplace_back(ptr);
    }

    return slice;
  }



  [[nodiscard]] output parse_output(const section& sec, opt_sec fallback) {
    auto [wp_parent, wallpaper_section] = select_section_sub(sec, fallback, "wallpaper");
    auto background_section = select_section_sub(sec, fallback, "background").second;

    std::vector<wallpaper> wallpapers{
      parse_wallpaper(wallpaper_section, background_section, false)
    };

    for (const auto* ptr: list_wallpaper_sections(*wp_parent)) {
      if (auto wp = parse_wallpaper(*ptr, background_section, true);
          !wp.condition.is_always_false()) {
        wallpapers.emplace_back(std::move(wp));
      }
    }

    std::ranges::reverse(wallpapers);

    auto panel_section = select_section_sub(sec, fallback, "panels").second;
    auto s_e_section   = select_section_sub(sec, fallback, "surface-effects").second;

    auto [rounded_corners, border_effects] = parse_surface_effects(s_e_section);

    bool clipping{false};
    if (auto key = sec.unique_key("clipping")) {
      clipping = parse<bool>(*key);
    } else if (fallback) {
      if (auto key = fallback->unique_key("clipping")) {
        clipping = parse<bool>(*key);
      }
    }

    return output {
      .name                 = std::string{sec.name()},
      .wallpapers           = std::move(wallpapers),
      .fixed_panels         = parse_panels(panel_section),
      .border_effects       = std::move(border_effects),
      .rounded_corners      = std::move(rounded_corners),
      .clipping             = clipping
    };
  }
}





cfg::config(std::string_view input) {
  try {
    auto root = parser::parse(input);

    update(root, poll_rate_,     "poll-rate-ms");
    update(root, fade_in_,       "fade-in-ms");
    update(root, fade_out_,      "fade-out-ms");

    update(root, disable_i3ipc_, "disable-i3ipc");

    update(root, as_overlay_,    "as-overlay");
    update(root, opacity_,       "opacity");



    default_output_ = parse_output(root, {});

    for (const auto& section: root.subsections()) {
      if (section.name().empty()                   ||
          section.name() == "panels"               ||
          section.name() == "wallpaper"            ||
          section.name() == "background"           ||
          section.name() == "surface-effects"      ||
          section.name().starts_with("wallpaper#")
      ) {
        continue;
      }

      outputs_.emplace_back(parse_output(section, root));
    }

    if (auto message = format_unused_message(root, input, logcerr::is_colored())) {
      logcerr::warn("config file contains unused keys");
      logcerr::print_raw_sync(std::cout, *message);
    }
  } catch (const iconfigp::exception& ex) {
    logcerr::error(ex.what());
    logcerr::print_raw_sync(std::cerr,
        format_exception(ex, input, logcerr::is_colored()));
    throw ::exception{"invalid config file"};
  }
}







output cfg::output_config_for(std::string_view name) const {
  if (auto it = std::ranges::find_if(outputs_,
      [name](const auto& out) { return out.name == name; }); it != outputs_.end()) {
    return *it;
  }

  output op{default_output_};
  op.name = name;
  return op;
}





namespace { namespace global_state {
  cfg global_config_;
}}

void ::config::global_config(config&& config) {
  global_state::global_config_ = std::move(config);
}

cfg& ::config::global_config() {
  return global_state::global_config_;
}
