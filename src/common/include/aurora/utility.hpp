/*
 * Copyright (C) 2021 fleroviux
 */

#pragma once

#include <functional>
#include <utility>

#define AURA_NO_COPY(T)\
  T(T const& other) = delete;\
  void operator=(T const& other) = delete;

#define AURA_NO_MOVE(T)\
  T(T&& other) = delete;\
  void operator=(T&& other) = delete;

#define AURA_NO_COPY_NO_MOVE(T)\
  AURA_NO_COPY(T)\
  AURA_NO_MOVE(T)

namespace Aura {

template<bool flag = false>
constexpr void static_no_match() {
  static_assert(flag);
}

struct pair_hash {
  template<typename T1, typename T2>
  auto operator()(std::pair<T1, T2> const& pair) const noexcept -> size_t {
    auto hash0 = std::hash<T1>{}(pair.first);
    auto hash1 = std::hash<T2>{}(pair.second);

    return hash1 ^ (0x9e3779b9 + (hash0 << 6) + (hash0 >> 2));
  }
};

} // namespace Aura

