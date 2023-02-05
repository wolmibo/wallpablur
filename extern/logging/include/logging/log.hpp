#ifndef LOGGING_LOG_HPP_INCLUDED
#define LOGGING_LOG_HPP_INCLUDED

#include <chrono>
#include <mutex>
#include <ostream>
#include <string_view>
#include <thread>

#include <version>
#if defined(__cpp_lib_format)
#include <format>
#else
#include <fmt/core.h>
#endif



namespace logging {

/// Statically check if debug entries should be created or discarded.
/// In order to show debug messages, output_level must be at least level::debug.
///
/// Define NDEBUG or LOGGING_DISABLE_DEBUGGING to disable debug entries,
/// LOGGING_ENABLE_DEBUGGING to enable debug entries.
/// Precedence order:
/// NDEBUG < LOGGING_ENABLE_DEBUGGING < LOGGING_DISABLE_DEBUGGING
[[nodiscard]] constexpr bool debugging_enabled() noexcept {
#if !defined(LOGGING_DISABLE_DEBUGGING) && \
    (!defined(NDEBUG) || defined(LOGGING_ENABLE_DEBUGGING))

  return true;
#else
  return false;
#endif
}





/// An enum describing strategies of when to use colored output.
enum class color_mode {
  never,
  auto_detect,
  always,
};

/// Checks if colors will be used according to the current color_mode and
/// the current terminal connected to the output.
[[nodiscard]] bool is_colored() noexcept;

/// Sets the current color_mode.
/// Use is_colored to determine if output will actually use colors.
///
/// @throws std::invalid_argument if mode is not a named value of color_mode
void colorize(color_mode mode);

/// Obtain the current color_mode.
/// Use is_colored to determine if output will actually use colors.
[[nodiscard]] color_mode colorize() noexcept;





/// An enum describing log message severities.
enum class severity {
  debug,
  verbose,
  log,
  warning,
  error,
};

/// Checks if a message of severity level will be printed with the current
/// settings.
[[nodiscard]] bool is_outputted(severity level) noexcept;

/// Prints all messages which have a severity of at least lowest_level.
void output_level(severity lowest_level) noexcept;

/// Returns what severity a message needs to have to be printed.
[[nodiscard]] severity output_level() noexcept;






/// Value to pass to merge_after to disable message merging
static constexpr size_t disable_merging{0};

/// Selects how often to print successive identical messages before showing a
/// counter. Two messages are considered equal if they have the same message,
/// severity and thread_name.
///
/// Use the disable_merging constant (or 0) to completely disable merging.
void merge_after(size_t merge) noexcept;

/// Checks the current merge strategy. If merging is disabled, disable_merging
/// will be returned.
[[nodiscard]] size_t merge_after() noexcept;

/// Interrupts the current chain of messages being merged by treating
/// the next message as being different from the previous own.
void interrupt_merging();





/// Obtains the current timestamp of the log in milliseconds.
[[nodiscard]] std::chrono::milliseconds elapsed();





/// Associates a thread_id with a human-readable name.
/// If a thread already had a name, it will be overwritten.
void thread_name(std::string_view name,
                 std::thread::id thread_id = std::this_thread::get_id());

/// Obtains the human-readable name associated with a thread_id.
/// If no name has been set yet, a serialized version of thread_id will be used.
[[nodiscard]] std::string
thread_name(std::thread::id thread_id = std::this_thread::get_id());





/// Acquires the output lock of this library to safely write across threads.
/// If the last write to std::cerr prior to calling this function was by a log
/// entry, the next character will be put on a new line and merging is
/// interrupted.
[[nodiscard]] std::unique_lock<std::mutex> output_lock();

/// Perform a write operation without interfering with other output performed by
/// this library. Same as writing message to out guarded by output_lock.
void print_raw_sync(std::ostream& out, std::string_view message);





namespace impl {
  void print(severity, std::string&&);
  void print_checked(severity, std::string&&);
  void print_checked(severity, std::string_view);

#if defined(__cpp_lib_format)
  namespace format = std;
#else
  namespace format = fmt;
#endif
}





/// Backend agnostic version of std::format_string / fmt::format_string
template <typename... Args>
using format_string = impl::format::format_string<Args...>;

/// Backend agnostic version of std::format / fmt::format
template <typename... Args>
std::string format(format_string<Args...> fmt, Args&&... args) {
  return impl::format::format(std::move(fmt), std::forward<Args>(args)...);
}




/// Print a message to std::cerr visible and formatted according to its severity
/// level.
template <typename... Args>
void print(severity level, format_string<Args...> fmt, Args&&... args) {
  if (is_outputted(level)) {
    impl::print(level,
                logging::format(std::move(fmt), std::forward<Args>(args)...));
  }
}





/// Append a debug message to the log.
/// If debugging_enabled() evalutes to true, this forwards to
///   print(severity::debug, fmt, args)
/// otherwise, this is a no-op.
template <typename... Args>
void debug(format_string<Args...> fmt, Args&&... args) {
  if constexpr (debugging_enabled()) {
    print(severity::debug, std::move(fmt), std::forward<Args>(args)...);
  }
}

inline void debug(std::string_view message) {
  if constexpr (debugging_enabled()) {
    impl::print_checked(severity::debug, message);
  }
}




/// Append a verbose message to the log.
/// Forwards to print(severity::verbose, fmt, args).
template <typename... Args>
void verbose(format_string<Args...> fmt, Args &&...args) {
  print(severity::verbose, std::move(fmt), std::forward<Args>(args)...);
}

inline void verbose(std::string_view message) {
  impl::print_checked(severity::verbose, message);
}



/// Append a log message to the log.
/// Forwards to print(severity::log, fmt, args).
template <typename... Args>
void log(format_string<Args...> fmt, Args &&...args) {
  print(severity::log, std::move(fmt), std::forward<Args>(args)...);
}

inline void log(std::string_view message) {
  impl::print_checked(severity::log, message);
}



/// Append a warning message to the log.
/// Forwards to print(severity::warning, fmt, args).
template <typename... Args>
void warn(format_string<Args...> fmt, Args&&... args) {
  print(severity::warning, std::move(fmt), std::forward<Args>(args)...);
}

inline void warn(std::string_view message) {
  impl::print_checked(severity::warning, message);
}



/// Append an error message to the log.
/// Forwards to print(severity::error, fmt, args).
template <typename... Args>
void error(format_string<Args...> fmt, Args&&... args) {
  print(severity::error, std::move(fmt), std::forward<Args>(args)...);
}

inline void error(std::string_view message) {
  impl::print_checked(severity::error, message);
}

}

#endif // LOGGING_LOG_HPP_INCLUDED
