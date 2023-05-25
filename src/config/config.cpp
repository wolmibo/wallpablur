#include "wallpablur/config/config.hpp"

#include <algorithm>
#include <string_view>



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
