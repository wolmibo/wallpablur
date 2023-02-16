#ifndef ICONFIGP_FORMAT_HPP_INCLUDED
#define ICONFIGP_FORMAT_HPP_INCLUDED

#include <version>
#include <chrono> // FIXME: to get __cpp_lib_format without compilation error pre GCC13

#if defined(__cpp_lib_format)
#include <format>
#else
#include <fmt/core.h>
#endif



namespace iconfigp {

namespace impl {
#if defined(__cpp_lib_format)
  namespace format = std;
#else
  namespace format = fmt;
#endif
}

template<typename... Args>
using format_string = impl::format::format_string<Args...>;

template<typename... Args>
std::string format(format_string<Args...> fmt, Args&&... args) {
  return impl::format::format(std::move(fmt), std::forward<Args>(args)...);
}


}

#endif // ICONFIGP_FORMAT_HPP_INCLUDED
