#include "wallpablur/config/config.hpp"
#include "wallpablur/config/filter.hpp"
#include "wallpablur/config/output.hpp"
#include "wallpablur/config/panel.hpp"

#include <iostream>
#include <optional>
#include <stdexcept>

#include <cassert>

#include <unistd.h>

#include <iconfigp/exception.hpp>
#include <iconfigp/parser.hpp>
#include <iconfigp/section.hpp>
#include <iconfigp/value-parser.hpp>

#include <logging/log.hpp>




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



template<> struct iconfigp::value_parser<std::filesystem::path> {
  static constexpr std::string_view name {"path"};
  static constexpr std::string_view format() {
    return "path where leading ~ gets replaced with the content of $HOME";
  }
  static std::optional<std::filesystem::path> parse(std::string_view input) {
    if (!input.starts_with("~/")) {
      return input;
    }

    input.remove_prefix(2);
    if (auto *home = getenv("HOME"); home != nullptr && *home != 0 && !input.empty()) {
      return std::filesystem::path{home} / input;
    }
    return {};
  }
};



template<> struct iconfigp::value_parser<config::panel::margin_type> {
  static constexpr std::string_view name {"margin"};
  static constexpr std::string_view format() {
    return "<int> or <left>:<right>:<top>:<bottom>";
  }
  static std::optional<config::panel::margin_type> parse(std::string_view input) {
    if (auto value = iconfigp::value_parser<int32_t>::parse(input)) {
      return config::panel::margin_type{
        .left   = *value,
        .right  = *value,
        .top    = *value,
        .bottom = *value
      };
    }
    auto list = split(input, ':');
    std::array<int32_t, 4> intlist{};

    if (list.size() != 4 || !parse_list<int32_t>(list, intlist)) {
      return {};
    }

    return config::panel::margin_type{
      .left   = intlist[0],
      .right  = intlist[1],
      .top    = intlist[2],
      .bottom = intlist[3]
    };
  }
};



template<> struct iconfigp::value_parser<config::panel::anchor_type> {
  static constexpr std::string_view name {"anchor"};
  static constexpr std::string_view format() {
    return "string made up of l, r, b and t";
  }
  static std::optional<config::panel::anchor_type> parse(std::string_view input) {
    config::panel::anchor_type anchor;
    for (auto character: input) {
      if      (character == 'l') { anchor.value |= config::panel::anchor_type::left;   }
      else if (character == 'r') { anchor.value |= config::panel::anchor_type::right;  }
      else if (character == 'b') { anchor.value |= config::panel::anchor_type::bottom; }
      else if (character == 't') { anchor.value |= config::panel::anchor_type::top;    }
      else {
        return {};
      }
    }
    return anchor;
  }
};



template<> struct iconfigp::value_parser<config::panel::size_type> {
  static constexpr std::string_view name {"size"};
  static constexpr std::string_view format() { return "<width>:<height>"; }

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



template<> struct iconfigp::value_parser<config::color> {
  static constexpr std::string_view name {"color"};
  static constexpr std::string_view format() {
    return "[#]rrggbb[aa] or [#]rgb[a] // case insensitive";
  }

  static std::optional<config::color> parse(std::string_view input) {
    if (input.starts_with('#')) {
      input.remove_prefix(1);
    }
    config::color out{0.f};
    size_t bpcc = input.size() / 4;
    if (bpcc * 4 == input.size()) {
      out[3] = hex_to_value(input.substr(3 * bpcc));
      input.remove_suffix(bpcc);
    } else {
      out[3] = 1.f;
    }

    bpcc = input.size() / 3;
    if (bpcc * 3 == input.size()) {
      out[0] = hex_to_value(input.substr(       0, bpcc));
      out[1] = hex_to_value(input.substr(    bpcc, bpcc));
      out[2] = hex_to_value(input.substr(2 * bpcc, bpcc));
      return out;
    }
    return {};
  }



  private:
    [[nodiscard]] static int hex_to_value(char c) {
      if ('0' <= c && c <= '9') { return c - '0'; }
      if ('a' <= c && c <= 'f') { return c - 'a' + 10; }
      if ('A' <= c && c <= 'F') { return c - 'A' + 10; }
      throw false;
    }

    [[nodiscard]] static float hex_to_value(std::string_view str) {
      if (str.size() == 2) {
        return (hex_to_value(str[0]) << 4 | hex_to_value(str[1])) / 255.f;
      }
      if (str.size() == 1) {
        return hex_to_value(str[0]) / 15.f;
      }
      throw false;
    }
};





namespace {
  [[nodiscard]] config::panel parse_panel(const iconfigp::group& group) {
    return config::panel {
      .anchor = iconfigp::parse<config::panel::anchor_type>(group.unique_key("anchor"))
                  .value_or(config::panel::anchor_type{}),
      .size   = iconfigp::parse<config::panel::size_type>  (group.unique_key("size"  ))
                  .value_or(config::panel::size_type{}),
      .margin = iconfigp::parse<config::panel::margin_type>(group.unique_key("margin"))
                  .value_or(config::panel::margin_type{})
    };
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
      case filter_e::box_blur:
        return config::box_blur_filter{
          .width = iconfigp::parse<uint32_t>(filter.unique_key("width"))
                     .or_else([&](){
                         return iconfigp::parse<uint32_t>(filter.unique_key("radius"));})
                     .value_or(96),
          .height = iconfigp::parse<uint32_t>(filter.unique_key("height"))
                     .or_else([&](){
                         return iconfigp::parse<uint32_t>(filter.unique_key("radius"));})
                     .value_or(96),

          .iterations = iconfigp::parse<uint32_t>(filter.unique_key("iterations"))
                          .value_or(filter_type == filter_e::blur ? 2 : 1)
        };
      case filter_e::invert:
        return config::invert_filter{};
    }
    throw std::runtime_error{"filter branch is not implemented"};
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
    return config::image_distribution {
      .scale  = iconfigp::parse<config::scale_mode>(section.unique_key("scale"))
                  .value_or(config::scale_mode::zoom),
      .wrap_x = iconfigp::parse<config::wrap_mode>(section.unique_key("wrap-x"))
                  .or_else([&](){
                    return iconfigp::parse<config::wrap_mode>(
                        section.unique_key("wrap")); })
                  .value_or(config::wrap_mode::none),
      .wrap_y = iconfigp::parse<config::wrap_mode>(section.unique_key("wrap-y"))
                  .or_else([&](){
                    return iconfigp::parse<config::wrap_mode>(
                        section.unique_key("wrap")); })
                  .value_or(config::wrap_mode::none)
    };
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

    auto panel_section = section.subsection("panels");
    if (!panel_section && fallback) {
      panel_section = fallback->subsection("panels");
    }

    return config::output {
      .name         = std::string{section.name()},
      .wallpaper    = std::move(wallpaper),
      .background   = std::move(background),
      .fixed_panels = parse_panels(panel_section)
    };
  }
}





config::config::config(std::string_view input) {
  try {
    auto root = iconfigp::parser::parse(input);

    poll_rate_ = std::chrono::milliseconds{
      iconfigp::parse<uint32_t>(root.unique_key("poll-rate-ms")).value_or(250)};

    fade_in_   = std::chrono::milliseconds{
      iconfigp::parse<uint32_t>(root.unique_key("fade-in-ms"  )).value_or(0)};

    fade_out_  = std::chrono::milliseconds{
      iconfigp::parse<uint32_t>(root.unique_key("fade-out-ms" )).value_or(0)};


    default_output_ = parse_output(root, {});

    for (const auto& section: root.subsections()) {
      if (section.name().empty()        ||
          section.name() == "panels"    ||
          section.name() == "wallpaper" ||
          section.name() == "background") {
        continue;
      }

      outputs_.emplace_back(parse_output(section, root));
    }

    if (auto message =
        iconfigp::generate_unused_message(root, input, logging::is_colored())) {

      logging::warn("config file contains unused keys");
      logging::print_raw_sync(std::cout, *message);
    }
  } catch (const iconfigp::exception& ex) {
    logging::error(ex.what());
    logging::print_raw_sync(std::cout,
        iconfigp::format_exception(ex, input, logging::is_colored()));
    throw false;
  }
}
