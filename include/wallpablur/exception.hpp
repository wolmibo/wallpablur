// Copyright (c) 2024 wolmibo
// SPDX-License-Identifier: MIT

#ifndef WALLPABLUR_EXCEPTION_HPP_INCLUDED
#define WALLPABLUR_EXCEPTION_HPP_INCLUDED

#include <optional>
#include <source_location>
#include <stdexcept>
#include <utility>


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








#endif // WALLPABLUR_EXCEPTION_HPP_INCLUDED
