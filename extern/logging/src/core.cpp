#include "logging/log.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <mutex>
#include <optional>
#include <span>
#include <sstream>
#include <vector>

#include <unistd.h>



namespace {
  [[nodiscard]] bool determine_colored() {
    return isatty(STDERR_FILENO) != 0;
  }
}





namespace { namespace global_state {
  auto                             start  {std::chrono::high_resolution_clock::now()};

  std::atomic<logging::color_mode> color  {logging::color_mode::auto_detect};
  std::atomic<bool>                colored{determine_colored()};

  std::atomic<logging::severity>   level  {logging::severity::debug};

  std::atomic<size_t>              merge  {2};

  std::mutex                       thread_names_mutex;
  std::vector<std::thread::id>     thread_ids  {std::this_thread::get_id()};
  std::vector<std::string>         thread_names{"main"};
}}





namespace {
  [[nodiscard]] std::string to_string(std::thread::id thread_id) {
    std::stringstream stream;
    stream << thread_id;
    return stream.str();
  }



  [[nodiscard]] std::optional<size_t> find_thread_index_unguarded(std::thread::id t_id)
  {
    auto iter = std::ranges::find(global_state::thread_ids, t_id);
    if (iter == global_state::thread_ids.end()) {
      return {};
    }

    return iter - global_state::thread_ids.begin();
  }
}





bool logging::is_colored() noexcept {
  return global_state::colored;
}

void logging::colorize(color_mode mode) {
  switch (mode) {
    case color_mode::never:
      global_state::colored = false;
      break;
    case color_mode::auto_detect:
      global_state::colored = determine_colored();
      break;
    case color_mode::always:
      global_state::colored = true;
      break;
    default:
      throw std::invalid_argument{"expected a vaild color mode"};
  }
  global_state::color = mode;
}

logging::color_mode logging::colorize() noexcept {
  return static_cast<logging::color_mode>(global_state::color.load());
}





void logging::output_level(severity lowest_level) noexcept {
  global_state::level = lowest_level;
}

logging::severity logging::output_level() noexcept {
  return global_state::level.load();
}

bool logging::is_outputted(severity lev) noexcept {
  return static_cast<std::underlying_type_t<severity>>(lev) >=
    static_cast<std::underlying_type_t<severity>>(output_level());
}





void logging::merge_after(size_t merge) noexcept {
  global_state::merge = merge;
}

size_t logging::merge_after() noexcept {
  return global_state::merge;
}





void logging::thread_name(std::string_view name, std::thread::id thread_id) {
  std::lock_guard<std::mutex> lock{global_state::thread_names_mutex};

  if (auto index = find_thread_index_unguarded(thread_id)) {
    global_state::thread_names[*index] = name;

  } else {
    global_state::thread_names.emplace_back(name);
    global_state::thread_ids.emplace_back(thread_id);
  }
}



std::string logging::thread_name(std::thread::id thread_id) {
  std::lock_guard<std::mutex> lock{global_state::thread_names_mutex};

  if (auto index = find_thread_index_unguarded(thread_id)) {
    return global_state::thread_names[*index];
  }

  global_state::thread_names.emplace_back(to_string(thread_id));
  global_state::thread_ids.emplace_back(thread_id);

  return global_state::thread_names.back();
}





std::chrono::milliseconds logging::elapsed() {
  return std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::high_resolution_clock::now() - global_state::start);
}
