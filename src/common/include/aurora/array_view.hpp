/*
 * Copyright (C) 2022 fleroviux
 */

#pragma once

#include <stddef.h>
#include <vector>

namespace Aura {

template<typename T>
struct ArrayView {
  using iterator = T*;
  using const_iterator = T const*;

  constexpr ArrayView(T* data, size_t size) : data_(data), size_(size) {}

  ArrayView(std::vector<T>& vec) : data_(vec.data()), size_(vec.size()) {}

  constexpr auto cbegin() const -> const_iterator {
    return data();
  }

  constexpr auto begin() -> iterator {
    return data();
  }

  constexpr auto cend() const -> const_iterator {
    return data() + size();
  }

  constexpr auto end() -> iterator {
    return data() + size();
  }

  constexpr auto data() const -> T const* {
    return data_;
  }

  constexpr auto data() -> T* {
    return data_;
  }

  constexpr auto size() const -> size_t {
    return size_;
  }

  constexpr bool empty() const {
    return size() != 0;
  }

  constexpr auto operator[](size_t i) const -> T const& {
    return data()[i];
  }

  constexpr auto operator[](size_t i) -> T& {
    return data()[i];
  }

private:
  T* data_;
  size_t size_;
};

}; // namespace Aura
