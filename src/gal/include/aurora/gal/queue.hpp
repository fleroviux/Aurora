
// Copyright (C) 2022 fleroviux. All rights reserved.

#pragma once

#include <array>
#include <aurora/gal/command_buffer.hpp>
#include <aurora/gal/fence.hpp>
#include <aurora/array_view.hpp>

namespace Aura {

struct Queue {
  virtual ~Queue() = default;

  virtual auto Handle() -> void* = 0;
  virtual void Submit(ArrayView<CommandBuffer*> buffers, AnyPtr<Fence> fence) = 0;

  //template<template <typename T> typename SmartPtr, size_t size>
  template<template<typename> typename SmartPtr>
  void Test(std::array<SmartPtr<CommandBuffer>, 2>& buffers) {

  }
};

} // namespace Aura
