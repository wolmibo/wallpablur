#include "iconfigp/section.hpp"
#include "iconfigp/serialize.hpp"
#include "iconfigp/format.hpp"

#include <algorithm>

#include <cctype>



namespace {
  [[nodiscard]] bool contains_any_of(std::string_view input, std::string_view search) {
    return std::ranges::any_of(input, [search](char c) {
        return search.find(c) != std::string_view::npos; });
  }
}



std::string iconfigp::serialize(std::string_view input, std::string_view illegal) {
  if (input.empty()) {
    return {};
  }

  if (!isspace(input.front()) && !isspace(input.back()) &&
      !contains_any_of(input, illegal)) {
    return std::string{input};
  }

  std::string output{"\""};

  for (char c: input) {
    if (c == '\\' || c == '\"') {
      output.push_back('\\');
      output.push_back(c);
    } else if (c == '\n') {
      output.push_back('\\');
      output.push_back('n');
    } else {
      output.push_back(c);
    }
  }

  output.push_back('"');

  return output;
}





std::string iconfigp::serialize(
    const section&   sec,
    size_t           indent,
    std::string_view parent
) {
  std::string ind(indent * 2, ' ');

  std::string name;
  if (!parent.empty()) {
    name += parent;
    name.push_back('.');
  }
  name += serialize(sec.name(), "\n\".]");

  auto output = format("{}[{}]\n", ind, name);

  bool written{false};
  for (const auto& grp: sec.groups()) {
    if (grp.empty()) {
      continue;
    }

    if (written) {
      output += ind + "-\n";
    }
    output += serialize(grp, indent);
    written = true;
  }

  for (const auto& subs: sec.subsections()) {
    if (written) {
      output.push_back('\n');
    }
    output += serialize(subs, indent + 1, name);
    written = true;
  }

  return output;
}





std::string iconfigp::serialize(const group& gp, size_t indent) {
  if (gp.empty()) {
    return "";
  }

  std::string output;
  for (const auto& kv: gp.entries()) {
    output += std::string(indent * 2, ' ') + serialize(kv) + "\n";
  }

  return output;
}





std::string iconfigp::serialize(const key_value& kv) {
  bool bad_start = kv.key().starts_with('-')
    || kv.key().starts_with('[')
    || kv.key().starts_with('#');

  return format("{} = {}",
      serialize(kv.key(), bad_start ? "\n\"=#;-[" : "\n\"=;"),
      serialize(kv.value(), "\n\";[")
  );
}
