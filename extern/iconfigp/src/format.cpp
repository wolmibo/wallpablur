#include "iconfigp/exception.hpp"
#include "iconfigp/format.hpp"
#include "iconfigp/located-string.hpp"
#include "iconfigp/section.hpp"
#include "iconfigp/serialize.hpp"

#include <optional>
#include <stdexcept>
#include <tuple>

#include <cstddef>



namespace {
  constexpr size_t line_width   = 90;
  constexpr int    color_red    = 31;
  constexpr int    color_yellow = 33;





  [[nodiscard]] std::tuple<std::string_view, size_t, size_t> find_line(
      std::string_view content,
      size_t           offset
  ) {
    if (offset >= content.size()) {
      throw std::out_of_range{"offset is outside of content"};
    }

    size_t line_number {1};

    for (auto pos = content.find('\n'); pos != std::string_view::npos;
        pos = content.find('\n')) {

      if (offset < pos) {
        return {content.substr(0, pos), line_number, offset};
      }

      if (offset == pos) {
        return {content.substr(0, pos), line_number, offset - 1};
      }

      content.remove_prefix(pos + 1);
      offset -= pos + 1;
      ++line_number;
    }
    return {content, line_number, offset};
  }





  [[nodiscard]] std::string dim(std::string&& content, bool colored) {
    if (!colored) {
      return content;
    }
    return "\x1b[2m" + content + "\x1b[0m";
  }



  [[nodiscard]] std::string colorize(std::string&& content, bool colored, int color) {
    if (!colored) {
      return content;
    }
    return iconfigp::format("\x1b[1;{}m{}\x1b[0m", color, content);
  }



  [[nodiscard]] std::string emphasize(std::string&& content, bool colored) {
    if (!colored) {
      return content;
    }
    return "\x1b[1;37m" + content + "\x1b[0m";
  }



  [[nodiscard]] std::string emphasize_range(
      std::string_view content,
      size_t           offset,
      size_t           count,
      bool             colored
  ) {
    if (!colored || count == 0 || offset >= content.size()) {
      return std::string{content};
    }

    std::string out;
    size_t end = offset + count;
    for (size_t i = 0; i < content.size();) {
      if (i == offset) { out += "\x1b[1;37m"; }
      out.push_back(content[i]);
      if (++i == end)  { out += "\x1b[0m"; }
    }
    return out;
  }





  [[nodiscard]] std::pair<std::string, size_t> center_offset(
      std::string_view line,
      size_t           offset,
      size_t           width
  ) {
    static constexpr size_t dot_size {3};
    static constexpr size_t min_size {dot_size * 2 + 1};
    static constexpr size_t rposition{5};

    if (width < min_size) {
      return {"", 0};
    }

    if (line.size() <= width) {
      return {std::string{line}, offset};
    }

    if (offset < width) {
      return {iconfigp::format("{}...", line.substr(0, width - dot_size)), offset};
    }

    if (line.size() - offset < width) {
      return {iconfigp::format("...{}", line.substr(line.size() - width + dot_size)),
        width + offset - line.size()};
    }

    size_t pos = width / rposition;

    return {iconfigp::format("...{}...",
        line.substr(offset + dot_size - pos, width - 2 * dot_size)), pos};
  }





  [[nodiscard]] std::string fallback_range(size_t offset, size_t count) {
    return iconfigp::format("  Position: {}{}\n", offset,
        (count > 0) ? iconfigp::format("-{}", offset + count) : "");
  }



  [[nodiscard]] std::string highlight_range(
      std::string_view source,
      size_t           offset,
      size_t           count,
      bool             colored,
      int              color
  ) {
    if (source.empty() || offset + count > source.size()) {
      return fallback_range(offset, count);
    }

    if (offset == source.size()) {
      offset--;
    }

    auto [whole_line, line_number, column] = find_line(source, offset);
    if (count > whole_line.size()) {
      count = 0;
    }

    auto line_prefix      = iconfigp::format("  {} | ", line_number);
    auto line_prefix_size = line_prefix.size();

    auto rem_width = line_width - line_prefix_size;

    auto [line, col] = center_offset(whole_line, column, rem_width);

    if (col + count > rem_width) {
      count = 0;
    }

    return iconfigp::format("{}{}\n{}{}\n",
        dim(std::move(line_prefix), colored),
        emphasize_range(line, col, count, colored),

        std::string(line_prefix_size + col, ' '),
        colorize(std::string(std::max<size_t>(count, 1), '^'), colored, color)
    );
  }





  [[nodiscard]] std::string format_missing_key(
    const iconfigp::missing_key_exception& ex,
    std::string_view                       source,
    bool                                   colored
  ) {
    return iconfigp::format("The required key {} is missing in this group:\n{}",
        emphasize(iconfigp::serialize(ex.key()), colored),
        highlight_range(source, ex.offset(), 0, colored, color_red)
    );
  }





  [[nodiscard]] std::string format_range(
    const iconfigp::value_parse_exception::range_exception& ex,
    std::string_view                                        source,
    bool                                                    colored
  ) {
    return iconfigp::format("{}:\n{}",
        ex.message(),
        highlight_range(source, ex.offset(), ex.size(), colored, color_red)
    );
  }



  [[nodiscard]] std::string format_value_parse(
    const iconfigp::value_parse_exception& ex,
    std::string_view                       source,
    bool                                   colored
  ) {
    return iconfigp::format("{} cannot be parsed as {}:\n{}{}"
        "A value of type {} has the following form:\n{}\n",
        ex.value().value().empty() ? "An empty value" :
          "The value " + emphasize(iconfigp::serialize(ex.value().value()), colored),

        emphasize(iconfigp::serialize(ex.target()), colored),

        highlight_range(source, ex.value().value_offset(),
          ex.value().value_size(), colored, color_red),

        ex.range_ex() ? (format_range(*ex.range_ex(), ex.value().value(), colored)) : "",

        iconfigp::serialize(ex.target()),
        ex.format()
      );
  }





  [[nodiscard]] std::string format_multiple_definitions(
    const iconfigp::multiple_definitions_exception& ex,
    std::string_view                                source,
    bool                                            colored
  ) {
    return iconfigp::format("The key {} is only allowed once per {}:\n{}"
        "Previous definition:\n{}{}",

        emphasize(iconfigp::serialize(ex.definition1().key()), colored),
        ex.per_section() ? "section" : "group",
        highlight_range(source, ex.definition2().key_offset(),
          ex.definition2().key_size(), colored, color_red),
        highlight_range(source, ex.definition1().key_offset(),
          ex.definition1().key_size(), colored, color_yellow),
        ex.per_section() ? "" : "Preceed the key with - to start a new group.\n"
    );
  }





  [[nodiscard]] std::string_view error_type_to_string(iconfigp::syntax_error_type type) {
    using enum iconfigp::syntax_error_type;
    switch (type) {
      case missing_quotation_mark:
      case missing_quotation_mark_eol:
        return "Missing a closing quotation mark";
      case invalid_escape_sequence:
        return "Invalid escape sequence";
      case unexpected_character:
        return "Unexpected character";

      case empty_key:
        return "Empty string";
      case unexpected_semicolon:
        return "Unexpected semicolon";
      case missing_section_end:
        return "Missing a matching ]";
      case missing_value:
        return "Missing a value";
    }
    return "Encountered a problem which apparently has no meaningful error message";
  }



  [[nodiscard]] size_t error_type_character_count(iconfigp::syntax_error_type type) {
    using enum iconfigp::syntax_error_type;
    switch (type) {
      case iconfigp::syntax_error_type::missing_quotation_mark:
      case iconfigp::syntax_error_type::missing_quotation_mark_eol:
      case iconfigp::syntax_error_type::invalid_escape_sequence:
      case iconfigp::syntax_error_type::unexpected_character:
      case iconfigp::syntax_error_type::unexpected_semicolon:
      case iconfigp::syntax_error_type::missing_section_end:
        return 1;
      case iconfigp::syntax_error_type::missing_value:
      case iconfigp::syntax_error_type::empty_key:
      default:
        return 0;
    }
  }



  [[nodiscard]] std::optional<std::string_view> error_type_hint(
      iconfigp::syntax_error_type type
  ) {
    using enum iconfigp::syntax_error_type;

    switch (type) {
      case missing_quotation_mark_eol:
        return "You can write multi-line strings by using the escape sequence \\n.\n";
      case invalid_escape_sequence:
        return "To escape '\\' use \\\\.\n";
      case unexpected_semicolon:
        return "Semicolons only belong right behind a value. Use # for comments.\n";
      case empty_key:
        return "Keys must not be empty.\n";
      case missing_value:
        return "Values are introduced using '='.\n";
      default:
        return {};
    }
  }



  [[nodiscard]] std::string_view task_type_to_string(iconfigp::task_type type) {
    using enum iconfigp::task_type;
    switch (type) {
      case key:      return "a key";
      case value:    return "a value";
      case section:  return "a section header";
      case toplevel: return "a toplevel element";
    }
    return "something";
  }



  [[nodiscard]] std::string format_syntax(
    const iconfigp::syntax_exception& ex,
    std::string_view                  source,
    bool                              colored
  ) {
    return iconfigp::format("{} while parsing {}:\n{}{}",
        error_type_to_string(ex.type()),
        task_type_to_string(ex.task()),
        highlight_range(source, ex.offset(), error_type_character_count(ex.type()),
          colored, color_red),
        error_type_hint(ex.type()).value_or("")
    );
  }
}





std::string iconfigp::format_exception(
  const exception& ex,
  std::string_view source,
  bool             colored
) {
  if (const auto* missing = dynamic_cast<const missing_key_exception*>(&ex)) {
    return format_missing_key(*missing, source, colored);
  }

  if (const auto* parse = dynamic_cast<const value_parse_exception*>(&ex)) {
    return format_value_parse(*parse, source, colored);
  }

  if (const auto* multi = dynamic_cast<const multiple_definitions_exception*>(&ex)) {
    return format_multiple_definitions(*multi, source, colored);
  }

  if (const auto* syntax = dynamic_cast<const syntax_exception*>(&ex)) {
    return format_syntax(*syntax, source, colored);
  }

  if (const auto* range =
      dynamic_cast<const value_parse_exception::range_exception*>(&ex)) {
    return format_range(*range, source, colored);
  }

  return ex.what();
}





std::optional<std::string> iconfigp::generate_unused_message(
    const section&   sec,
    std::string_view source,
    bool             colored
) {
  std::string output;

  auto unused_sections = sec.unused_sections();
  if (!unused_sections.empty()) {
    if (unused_sections.size() > 1) {
      output += "The following sections (and their key(s)) have not been used:\n";
    } else {
      output += "The following section (and its key(s)) has not been used:\n";
    }
    for (const auto* un_sec: unused_sections) {
      if (source.empty()) {
        output += iconfigp::format("  {}\n", un_sec->name());
      } else {
        output += highlight_range(source, un_sec->offset(), 0, colored, color_yellow);
      }
    }
  }



  auto unused_keys = sec.unused_keys();
  if (!unused_keys.empty()) {
    if (unused_keys.size() > 1) {
      output += "The following keys have not been used:\n";
    } else {
      output += "The following key has not been used:\n";
    }
    for (const auto* un_key: unused_keys) {
      if (source.empty()) {
        output += iconfigp::format("  {}\n", un_key->key());
      } else {
        output += highlight_range(source,
            un_key->key_offset(), un_key->key_size(), colored, color_yellow);
      }
    }
  }



  if (output.empty()) {
    return {};
  }

  std::string pron = (unused_sections.size() + unused_keys.size() > 1)
                       ? "them" : "it";

  output += "Make sure you have spelled " + pron + " correctly.\n"
    "Alternatively, you can use # to comment out unneeded keys.\n";

  return output;
}
