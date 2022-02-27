/*
 * Copyright (C) 2021 fleroviux
 */

#pragma once

#include <aurora/renderer/gpu_resource.hpp>
#include <aurora/array_view.hpp>
#include <aurora/integer.hpp>
#include <vector>

namespace Aura {

// TODO: move this to a more appropriate location.
enum class VertexDataType {
  SInt8,
  UInt8,
  SInt16,
  UInt16,
  UInt32,
  Float16,
  Float32
};

struct VertexBuffer final : GPUResource {
  VertexBuffer(
    size_t stride,
    std::vector<u8>&& buffer
  ) : stride_(stride), buffer_(std::move(buffer)) {}

  VertexBuffer(
    size_t stride,
    size_t vertex_count
  ) : stride_(stride) {
    buffer_.resize(stride_ * vertex_count);
  }

  auto stride() const -> size_t {
    return stride_;
  }

  auto data() const -> u8 const* {
    return buffer_.data();
  }

  auto data() -> u8* {
    return buffer_.data();
  }

  auto size() const -> size_t {
    return buffer_.size();
  }

  template<typename T>
  auto view() const -> ArrayView<T const> {
    return ArrayView<T const>{(T*)data(), size() / sizeof(T)};
  }

  template<typename T>
  auto view() -> ArrayView<T> {
    return ArrayView<T>{(T*)data(), size() / sizeof(T)};
  }

  template<typename T>
  auto read(size_t id, size_t offset = 0, size_t component = 0) -> T& {
    return ((T*)(data() + id * stride() + offset))[component];
  }

  template<typename T>
  auto read(size_t id, size_t offset = 0, size_t component = 0) const -> T {
    return const_cast<VertexBuffer*>(this)->read<T>(id, offset, component);
  }

  template<typename T>
  void write(size_t id, T value, size_t offset = 0, size_t component = 0) {
    ((T*)(data() + id * stride() + offset))[component] = value;
  }

private:
  size_t stride_;
  std::vector<u8> buffer_;
};

} // namespace Aura
