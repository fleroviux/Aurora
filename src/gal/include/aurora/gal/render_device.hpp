/*
 * Copyright (C) 2022 fleroviux
 */

#pragma once

#include <aurora/gal/buffer.hpp>
#include <aurora/array_view.hpp>
#include <cstring>
#include <memory>
#include <vector>

namespace Aura {
 
struct RenderDevice {
  virtual ~RenderDevice() = default;

  virtual auto Handle() -> void* = 0;
  
  virtual auto CreateBuffer(
    BufferUsage usage,
    size_t size,
    bool host_visible = true,
    bool map = true
  ) -> std::unique_ptr<Buffer> = 0;

  template<typename T>
  auto CreateBufferWithData(
    BufferUsage usage,
    T const* data,
    size_t size,
    bool unmap = true
  ) -> std::unique_ptr<Buffer> {
    auto buffer = CreateBuffer(usage | BufferUsage::TransferDst, size);

    std::memcpy(buffer->Data(), data, size);
    buffer->Flush();

    if (unmap) {
      buffer->Unmap();
    }

    return buffer;
  }

  template<typename T>
  auto CreateBufferWithData(
    BufferUsage usage,
    ArrayView<T> const& data,
    bool unmap = true
  ) -> std::unique_ptr<Buffer> {
    return CreateBufferWithData(usage, data.data(), data.size() * sizeof(T), unmap);
  }

  template<typename T>
  auto CreateBufferWithData(
    BufferUsage usage,
    std::vector<T> const& data,
    bool unmap = true
  ) -> std::unique_ptr<Buffer> {
    return CreateBufferWithData(usage, data.data(), data.size() * sizeof(T), unmap);
  }
};

} // namespace Aura
