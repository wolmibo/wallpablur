// Copyright (c) 2023 wolmibo
// SPDX-License-Identifier: MIT

#ifndef WALLPABLUR_FLAGS_HPP_INCLUDED
#define WALLPABLUR_FLAGS_HPP_INCLUDED

#include <bitset>
#include <type_traits>
#include <utility>



template<typename Enum>
  requires std::is_scoped_enum_v<Enum>
    && std::is_same_v<std::underlying_type_t<Enum>, size_t>
using flag_mask = std::bitset<std::to_underlying(Enum::eoec_marker)>;



template<typename Enum>
constexpr void set_flag(flag_mask<Enum>& mask, Enum flag, bool value = true) {
  mask[std::to_underlying(flag)] = value;
}



template<typename Enum>
[[nodiscard]] constexpr bool test_flag(const flag_mask<Enum>& mask, Enum flag) {
  return mask[std::to_underlying(flag)];
}



#endif // WALLPABLUR_FLAGS_HPP_INCLUDED
