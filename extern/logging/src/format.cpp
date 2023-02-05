#include "logging/log.hpp"

#include <iostream>
#include <mutex>
#include <optional>
#include <span>
#include <vector>

#if !defined(__cpp_lib_format)
#include <fmt/chrono.h>
#endif





namespace { namespace global_state {
  std::mutex output_mutex;

  bool       last_message_returned{false}; // guarded by output_mutex



  class terminator_t {
    public:
      terminator_t(const terminator_t&) = delete;
      terminator_t(terminator_t&&)      = delete;
      terminator_t& operator=(const terminator_t&) = delete;
      terminator_t& operator=(terminator_t&&)      = delete;

      terminator_t() = default;

      ~terminator_t() {
        logging::interrupt_merging();
      }
  } terminator;
}}





namespace {
  [[nodiscard]] std::vector<std::string_view> split(std::string_view input) {
    if (input.empty()) {
      return {input};
    }

    std::vector<std::string_view> output;

    while (true) {
      auto pos = input.find('\n');
      output.emplace_back(input.substr(0, pos));

      if (pos == std::string_view::npos) {
        break;
      }

      input.remove_prefix(pos + 1);
    }

    while (output.size() > 1 && output.back().empty()) {
      output.pop_back();
    }

    return output;
  }





  void write(std::ostream& out, std::string_view message) {
    out.write(message.data(), std::ssize(message));
  }



  template<typename... Args>
  [[nodiscard]] std::string format_var(std::string_view fmt, Args&&... args) {
    #if defined(__cpp_lib_format)
      return std::vformat(fmt, std::make_format_args(std::forward<Args>(args)...));
    #else
      return fmt::format(fmt::runtime(fmt), std::forward<Args>(args)...);
    #endif
  }





  [[nodiscard]] std::string_view format_extra(bool use_color, logging::severity level) {
    if (!use_color) {
      return "\r  | {}{}";
    }

    switch (level) {
      case logging::severity::error:
        return "\r  \x1b[2m|\x1b[0m \x1b[1m{}{}\x1b[0m";
      case logging::severity::debug:
        return "\r  \x1b[2m| {}{}\x1b[0m";
      default:
        return "\r  \x1b[2m|\x1b[0m {}{}";
    }
  }





  [[nodiscard]] std::string_view format_main(bool use_color, logging::severity level) {
    if (!use_color) {
      switch (level) {
        case logging::severity::warning:
          return "\r[{:%H:%M:%S} {}] [Warning] {}{}";
        case logging::severity::error:
          return "\r[{:%H:%M:%S} {}] *[Error]* {}{}";
        default:
          return "\r[{:%H:%M:%S} {}] {}{}";
      }
    }

    switch (level) {
      case logging::severity::debug:
        return "\r\x1b[2m[{:%H:%M:%S} {}] {}{}\x1b[0m";
      case logging::severity::warning:
        return "\r[{:%H:%M:%S} {}] \x1b[33m[Warning]\x1b[0m {}{}";
      case logging::severity::error:
        return "\r\x1b[1m[{:%H:%M:%S} {}] \x1b[31m*[Error]*\x1b[39m {}{}\x1b[0m";
      default:
        return "\r[{:%H:%M:%S} {}] {}{}";
    }
  }





  void print_message_unguarded(
    logging::severity                 level,
    std::span<const std::string_view> lines,
    std::string_view                  thread_name,
    std::string_view                  terminal
  ) {
    bool colored = logging::is_colored();

    for (auto it = lines.begin(); it != lines.end(); ++it) {
      auto term = (it + 1) == lines.end() ? terminal : std::string_view{"\n"};

      if (it == lines.begin()) {
        write(std::cerr, format_var(format_main(colored, level),
                                    logging::elapsed(), thread_name, *it, term));
      } else {
        write(std::cerr, format_var(format_extra(colored, level), *it, term));
      }
    }
  }





  class entry {
    public:
      entry(const entry&) = delete;

      entry(entry&& rhs) noexcept :
        level      {rhs.level},
        message    {std::move(rhs.message)},
        thread_name{std::move(rhs.thread_name)},
        lines      {split(message)}
      {}

      entry& operator=(const entry&) = delete;

      entry& operator=(entry&& rhs) noexcept {
        if (*this == rhs) {
          count++;
        } else {
          level       = rhs.level;
          message     = std::move(rhs.message);
          thread_name = std::move(rhs.thread_name);
          lines       = split(message);
          count = 1;
        }

        return *this;
      }

      ~entry() = default;

      entry(logging::severity lev, std::string&& msg) :
        level      {lev},
        message    {std::move(msg)},
        thread_name{logging::thread_name()},
        lines      {split(message)}
      {}



      [[nodiscard]] bool operator==(const entry& rhs) const {
        return level     == rhs.level
          && message     == rhs.message
          && thread_name == rhs.thread_name;
      };



      void print_unguarded(size_t merge) const {
        if (count <= merge) {
          if (global_state::last_message_returned) {
            std::cerr.put('\n');
          }

          print_message_unguarded(level, lines, thread_name, "");
        } else {
          if (lines.size() == 1) {
            write(std::cerr, format_var(format_main(logging::is_colored(), level),
                                        logging::elapsed(), thread_name, lines.front(),
                                        format_counter(merge)));
          } else if (lines.size() > 1) {
            write(std::cerr, format_var(format_extra(logging::is_colored(), level),
                                        lines.back(), format_counter(merge)));
          }
        }

        global_state::last_message_returned = true;
      }



    private:
      logging::severity level;
      std::string       message;
      std::string       thread_name;
      size_t            count{1};

      std::vector<std::string_view> lines;



      [[nodiscard]] std::string format_counter(size_t coalesce) const {
        return logging::format(" (x{})", count + 1 - coalesce);
      }
  };
}



namespace { namespace global_state {
  std::optional<entry> last_message; // guarded by output_mutex
}}





namespace {
  void interrupt_merging_unguarded() {
    global_state::last_message = {};
    if (global_state::last_message_returned) {
      std::cerr.put('\n');
      global_state::last_message_returned = false;
    }
  }



  void basic_print(logging::severity level, std::string&& message) {
    std::lock_guard<std::mutex> lock{global_state::output_mutex};

    if (auto merge = logging::merge_after(); merge > 0) {
      global_state::last_message = entry{level, std::move(message)};
      global_state::last_message->print_unguarded(merge);
    } else {
      print_message_unguarded(level, split(message), logging::thread_name(), "\n");
      global_state::last_message_returned = false;
    }
  }
}





void logging::impl::print(severity level, std::string&& message) {
  basic_print(level, std::move(message));
}

void logging::impl::print_checked(severity level, std::string_view message) {
  if (is_outputted(level)) {
    basic_print(level, std::string{message});
  }
}





void logging::interrupt_merging() {
  std::lock_guard<std::mutex> lock{global_state::output_mutex};

  interrupt_merging_unguarded();
}



void logging::print_raw_sync(std::ostream& out, std::string_view message) {
  std::lock_guard<std::mutex> lock{global_state::output_mutex};

  interrupt_merging_unguarded();

  out.write(message.data(), std::ssize(message));
}



std::unique_lock<std::mutex> logging::output_lock() {
  std::unique_lock<std::mutex> lock{global_state::output_mutex};

  interrupt_merging_unguarded();

  return lock;
}
