/*
 * Copyright (C) 2022 fleroviux
 */

#pragma once

#include <memory>

namespace Aura {

/**
 * Can be constructed from a raw pointer, unique_ptr or shared_ptr.
 * Holds the pointer to the underlying object.
 * Use this when you want to accept both a unique_ptr or shared_ptr reference
 * as a function argument.
 */
template<typename T>
struct AnyPtr {
  AnyPtr(T* ptr = nullptr) : ptr(ptr) {}
  AnyPtr(std::unique_ptr<T> const& smart_ptr) : ptr(smart_ptr.get()) {}
  AnyPtr(std::shared_ptr<T> const& smart_ptr) : ptr(smart_ptr.get()) {}

  auto get() -> T* {
    return ptr;
  }

  auto operator->() -> T* {
    return ptr;
  }

  auto operator*() -> T& {
    return *ptr;
  }

private:
  T* ptr;
};

} // namespace Aura
