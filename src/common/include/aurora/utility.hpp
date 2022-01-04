/*
 * Copyright (C) 2021 fleroviux
 */

#pragma once

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

} // namespace Aura

