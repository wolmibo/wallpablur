#include "wallpablur/config/border-effect.hpp"
#include "wallpablur/config/config.hpp"
#include "wallpablur/config/filter.hpp"
#include "wallpablur/config/output.hpp"
#include "wallpablur/config/panel.hpp"
#include "wallpablur/config/types.hpp"

#include <chrono>
#include <iostream>
#include <optional>
#include <stdexcept>

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

using opt_sec = iconfigp::opt_ref<const iconfigp::section>;




template<> struct iconfigp::case_insensitive_parse_lut<wrap_mode> {
  static constexpr std::string_view name {"wrap-mode"};
  static constexpr std::array<std::pair<std::string_view, wrap_mode>, 4> lut {
    std::make_pair("none",         wrap_mode::none),
    std::make_pair("stretch-edge", wrap_mode::stretch_edge),
    std::make_pair("tiled",        wrap_mode::tiled),
    std::make_pair("tiled-mirror", wrap_mode::tiled_mirror),
  };
};

template<> struct iconfigp::case_insensitive_parse_lut<scale_mode> {
  static constexpr std::string_view name {"scale-mode"};
  static constexpr std::array<std::pair<std::string_view, scale_mode>, 4> lut {
    std::make_pair("fit",      scale_mode::fit),
    std::make_pair("zoom",     scale_mode::zoom),
    std::make_pair("stretch",  scale_mode::stretch),
    std::make_pair("centered", scale_mode::centered),
  };
};

template<> struct iconfigp::case_insensitive_parse_lut<scale_filter> {
  static constexpr std::string_view name{"scale-filter"};
  static constexpr std::array<std::pair<std::string_view, scale_filter>, 2> lut {
    std::make_pair("linear",  scale_filter::linear),
    std::make_pair("nearest", scale_filter::nearest)
  };
};

template<> struct iconfigp::case_insensitive_parse_lut<blend_mode> {
  static constexpr std::string_view name{"blend-mode"};
  static constexpr std::array<std::pair<std::string_view, blend_mode>, 3> lut {
    std::make_pair("add",     blend_mode::add),
    std::make_pair("alpha",   blend_mode::alpha),
    std::make_pair("replace", blend_mode::replace)
  };
};

template<> struct iconfigp::case_insensitive_parse_lut<falloff> {
  static constexpr std::string_view name{"falloff"};
  static constexpr std::array<std::pair<std::string_view, falloff>, 3> lut {
    std::make_pair("none",       falloff::none),
    std::make_pair("linear",     falloff::linear),
    std::make_pair("sinusoidal", falloff::sinusoidal)
  };
};

template<> struct iconfigp::case_insensitive_parse_lut<border_position> {
  static constexpr std::string_view name{"border-position"};
  static constexpr std::array<std::pair<std::string_view, border_position>, 3>
  lut {
    std::make_pair("outside",  border_position::outside),
    std::make_pair("inside",   border_position::inside),
    std::make_pair("centered", border_position::centered)
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

static const sides_type sides_type_all_sides {
  .absolute = anchor_type::all()
};

static const border_effect surface_effect_e_border {
  .condition = true,
  .thickness = 2,
  .position  = border_position::outside,
  .offset    = border_effect::offset_type{},
  .col       = {0.f, 0.f, 0.f, 1.f},
  .blend     = blend_mode::alpha,
  .foff      = falloff::none,
  .exponent  = 1.f,
  .sides     = sides_type_all_sides
};

static const border_effect surface_effect_e_shadow {
  .condition = true,
  .thickness = 30,
  .position  = border_position::centered,
  .offset    = border_effect::offset_type {.x = 2, .y = 2},
  .col       = {0.f, 0.f, 0.f, 0.8f},
  .blend     = blend_mode::alpha,
  .foff      = falloff::sinusoidal,
  .exponent  = 1.5f,
  .sides     = sides_type_all_sides
};

static const border_effect surface_effect_e_glow {
  .condition = true,
  .thickness = 20,
  .position  = border_position::outside,
  .offset    = border_effect::offset_type{},
  .col       = {1.f, 1.f, 1.f, 1.f},
  .blend     = blend_mode::add,
  .foff      = falloff::linear,
  .exponent  = 3.f,
  .sides     = sides_type_all_sides
};

namespace {
  [[nodiscard]] const border_effect& surface_effect_e_default(surface_effect_e var) {
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



template<> struct iconfigp::value_parser<margin_type> {
  static constexpr std::string_view name {"margin"};
  static constexpr std::string_view format() {
    return "<all:i32> or <left:i32>:<right:i32>:<top:i32>:<bottom:i32>";
  }
  static std::optional<margin_type> parse(std::string_view input) {
    return std::bit_cast<margin_type>(parse_as_array<int32_t, 4>(input));
  }
};



template<> struct iconfigp::value_parser<anchor_type> {
  static constexpr std::string_view name {"anchor"};
  static constexpr std::string_view format() {
    return "string made up of l, r, b and t";
  }
  static std::optional<anchor_type> parse(std::string_view input) {
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
  }
};



template<> struct iconfigp::value_parser<sides_type> {
  static constexpr std::string_view name {"sides"};
  static constexpr std::string_view format() {
    return "string made up of l, r, b, t, n, e, s, and w";
  }
  static std::optional<sides_type> parse(std::string_view input) {
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
    return sides_type{relative, absolute};
  }
};



template<> struct iconfigp::value_parser<panel::size_type> {
  static constexpr std::string_view name {"size"};
  static constexpr std::string_view format() { return "<width:u32>:<height:u32>"; }

  static std::optional<panel::size_type> parse(std::string_view input) {
    return std::bit_cast<panel::size_type>(parse_as_array<uint32_t, 2>(input));
  }
};



template<> struct iconfigp::value_parser<border_effect::offset_type> {
  static constexpr std::string_view name {"size"};
  static constexpr std::string_view format() { return "<x:i32>,<y:i32>"; }

  static std::optional<border_effect::offset_type> parse(std::string_view input) {
    return std::bit_cast<border_effect::offset_type>(parse_as_array<int32_t, 2>(input));
  }
};





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



  [[nodiscard]] std::vector<border_effect> parse_border_effects(opt_sec sec) {
    if (!sec) {
      return {};
    }

    std::vector<border_effect> effects;
    for (const auto& grp: sec->groups()) {
      if (auto type = grp.unique_key("type")) {
        if (auto effect = parse_border_effect(grp,
                            surface_effect_e_default(parse<surface_effect_e>(*type)));
            !effect.condition.is_always_false()) {
          effects.emplace_back(std::move(effect));
        }
      }
    }
    return effects;
  }





  [[nodiscard]] panel parse_panel(const group& grp) {
    panel output;
    update(grp, output.anchor,    "anchor");
    update(grp, output.size,      "size");
    update(grp, output.margin,    "margin");
    update(grp, output.radius,    "border-radius");
    update(grp, output.app_id,    "app-id", "app_id"sv);
    update(grp, output.focused,   "focused");
    update(grp, output.urgent,    "urgent");
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
    auto filter_type = parse<filter_e>(filter.require_unique_key("filter"));

    switch (filter_type) {
      case filter_e::blur:
      case filter_e::box_blur: {
        box_blur_filter output;
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
        return invert_filter{};
    }
    throw std::runtime_error{"filter type is not implemented"};
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



  [[nodiscard]] opt_sec best_subsection(
      const section&   sec,
      opt_sec          fallback,
      std::string_view name
  ) {
    auto cand = sec.subsection(name);
    if (!cand && fallback) {
      return fallback->subsection(name);
    }
    return cand;
  }



  [[nodiscard]] wallpaper parse_wallpaper(opt_sec sec, opt_sec bg_sec) {
    wallpaper wp{};

    wp.description = parse_brush(sec, {});

    if (sec) {
      update(*sec, wp.condition, "enable-if");
    }

    wp.background.description = parse_brush(bg_sec, wp.description);

    if (bg_sec) {
      update(*bg_sec, wp.background.condition, "enable-if");
    }

    return wp;
  }



  [[nodiscard]] std::vector<const section*> list_wallpaper_sections(
      const section& sec,
      opt_sec        fallback
  ) {
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
    auto wallpaper_section  = best_subsection(sec, fallback, "wallpaper");
    auto background_section = best_subsection(sec, fallback, "background");

    std::vector<wallpaper> wallpapers{
      parse_wallpaper(wallpaper_section, background_section)
    };

    for (const auto* ptr: list_wallpaper_sections(sec, fallback)) {
      if (auto wp = parse_wallpaper(*ptr, background_section);
          !wp.condition.is_always_false()) {
        wallpapers.emplace_back(std::move(wp));
      }
    }

    std::ranges::reverse(wallpapers);

    auto panel_section          = best_subsection(sec, fallback, "panels");
    auto border_effects_section = best_subsection(sec, fallback, "surface-effects");

    return output {
      .name                 = std::string{sec.name()},
      .wallpapers           = std::move(wallpapers),
      .fixed_panels         = parse_panels(panel_section),
      .border_effects       = parse_border_effects(border_effects_section)
    };
  }
}





::config::config::config(std::string_view input) {
  try {
    auto root = parser::parse(input);

    update(root, poll_rate_,     "poll-rate-ms");
    update(root, fade_in_,       "fade-in-ms");
    update(root, fade_out_,      "fade-out-ms");

    update(root, disable_i3ipc_, "disable-i3ipc");

    update(root, as_overlay_,    "as-overlay");
    update(root, opacity_,       "opacity");
    update(root, gl_samples_,    "gl-samples");



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

    if (auto message =
        generate_unused_message(root, input, logcerr::is_colored())) {

      logcerr::warn("config file contains unused keys");
      logcerr::print_raw_sync(std::cout, *message);
    }
  } catch (const exception& ex) {
    logcerr::error(ex.what());
    logcerr::print_raw_sync(std::cout,
        format_exception(ex, input, logcerr::is_colored()));
    throw false;
  }
}
