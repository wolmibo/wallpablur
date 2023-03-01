#include "wallpablur/config/config.hpp"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <stdexcept>
#include <string_view>

#include <unistd.h>



namespace {
  [[nodiscard]] std::optional<std::string_view> find_env(const char* name) {
    if (const char* val = getenv(name); val != nullptr && *val != 0) {
      return val;
    }
    return {};
  }



  [[nodiscard]] std::optional<std::filesystem::path> only_existing(
      const std::filesystem::path&& path
  ) {
    if (std::filesystem::exists(path)) {
      return path;
    }
    return {};
  }
}



std::optional<std::filesystem::path> config::find_config_file() {
  if (auto val = find_env("WALLPABLUR_CONFIG")) {
    if (auto path = only_existing(*val)) {
      return path;
    }
  }

  const std::filesystem::path tail{"wallpablur/config.ini"};

  if (auto config = find_env("XDG_CONFIG_HOME")) {
    if (auto path = only_existing(*config / tail)) {
      return path;
    }
  }

  if (auto home = find_env("HOME")) {
    if (auto path = only_existing(std::filesystem::path{*home} / ".config" / tail)) {
      return path;
    }
  }

  return {};
}




config::output config::config::output_config_for(std::string_view name) const {
  if (auto it = std::ranges::find_if(outputs_,
      [name](const auto& out) { return out.name == name; }); it != outputs_.end()) {
    return *it;
  }

  output op{default_output_};
  op.name = name;
  return op;
}





namespace { namespace global_state {
  config::config global_config_;
}}

void config::global_config(config&& config) {
  global_state::global_config_ = std::move(config);
}

config::config& config::global_config() {
  return global_state::global_config_;
}
