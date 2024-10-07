// Copyright (c) 2024 wolmibo
// SPDX-License-Identifier: MIT

#ifndef WALLPABLUR_EXCEPTION_HPP_INCLUDED
#define WALLPABLUR_EXCEPTION_HPP_INCLUDED

#include <format>
#include <functional>
#include <optional>
#include <source_location>
#include <stdexcept>
#include <system_error>
#include <utility>

#include <logcerr/log.hpp>


class exception: public std::runtime_error {
  public:
    explicit exception(
        const std::string&                        message,
        bool                                      recoverable = false,
        std::optional<const std::source_location> location    = std::source_location::current()
    ) :
      std::runtime_error{message},
      recoverable_      {recoverable},
      location_         {std::move(location)}
    {}



    [[nodiscard]] bool recoverable() const { return recoverable_; }

    [[nodiscard]] const std::optional<const std::source_location>& location() const {
      return location_;
    } 



  private:
    bool                                      recoverable_;
    std::optional<const std::source_location> location_;
};





template<typename Fnc, typename Str, typename Act>
void check_errno_then(
    Str&& message,
    Fnc&& fnc,
    Act&& action,
    std::source_location location = std::source_location::current()
) {
  errno = 0;

  if (std::invoke(std::forward<Fnc>(fnc))) {
    return;
  }

  std::string msg{};

  if (auto error = std::make_error_code(static_cast<std::errc>(errno))) {
    msg = std::format("{}: {} ({})", std::forward<Str>(message), error.message(), error.value());
  } else {
    msg = std::format("{}: unknown error", std::forward<Str>(message));
  }

  std::invoke(std::forward<Act>(action), std::move(msg), std::move(location));
}



template<typename Fnc, typename Str>
void check_errno(
    Str&& message,
    Fnc&& fnc,
    std::source_location location = std::source_location::current()
) {
  check_errno_then(
      std::forward<Str>(message),
      std::forward<Fnc>(fnc),
      [](const std::string& str, std::source_location loc) {
        throw exception(str, true, loc);
      },
      location
  );
}



template<typename Fnc, typename Str>
void check_errno_nothrow(
    Str&& message,
    Fnc&& fnc,
    std::source_location location = std::source_location::current()
) {
  check_errno_then(
      std::forward<Str>(message),
      std::forward<Fnc>(fnc),
      [](const std::string& str, std::source_location loc) {
        logcerr::error("{}\n{}:{} in {}", str,
            loc.file_name(), loc.line(), loc.function_name());
      },
      location
  );
}


#endif // WALLPABLUR_EXCEPTION_HPP_INCLUDED
