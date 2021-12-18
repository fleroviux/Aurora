/*
 * Copyright (C) 2021 fleroviux
 */

#pragma once

#include <array>
#include <fmt/format.h>
#include <string_view>
#include <utility>

namespace Aura {

enum Level {
  Trace =  1,
  Debug =  2,
  Info  =  4,
  Warn  =  8,
  Error = 16,
  Fatal = 32,

  All = Trace | Debug | Info | Warn | Error | Fatal
};

namespace detail {

#if defined(NDEBUG)
  static constexpr int kLogMask = Info | Warn | Error | Fatal;
#else
  static constexpr int kLogMask = All;
#endif

} // namespace Aura::detail

template<Level level, typename... Args>
inline void Log(std::string_view format, Args... args) {
  if constexpr((detail::kLogMask & level) != 0) {
    char const* prefix = "[?]";

    if constexpr(level == Trace) prefix = "\e[36m[T]";
    if constexpr(level == Debug) prefix = "\e[34m[D]";
    if constexpr(level ==  Info) prefix = "\e[37m[I]";
    if constexpr(level ==  Warn) prefix = "\e[33m[W]";
    if constexpr(level == Error) prefix = "\e[35m[E]";
    if constexpr(level == Fatal) prefix = "\e[31m[F]";

    fmt::print("{} {}\n", prefix, fmt::format(format, args...));
  }
}

template<typename... Args>
inline void Assert(bool condition, Args... args) {
#if defined(DEBUG)
  if (!condition) {
    Log<Fatal>(args...);
    std::exit(-1);
  }
#endif
}

} // namespace Aura
