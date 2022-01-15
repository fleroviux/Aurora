/*
 * Copyright (C) 2022 fleroviux
 */

#pragma once

#include <aurora/gal/buffer.hpp>
#include <aurora/gal/render_target.hpp>
#include <aurora/gal/shader_module.hpp>
#include <aurora/gal/texture.hpp>
#include <aurora/array_view.hpp>
#include <aurora/integer.hpp>
#include <cstring>
#include <memory>
#include <vector>

namespace Aura {
 
struct RenderDevice {
  virtual ~RenderDevice() = default;

  virtual auto Handle() -> void* = 0;
  
  virtual auto CreateBuffer(
    Buffer::Usage usage,
    size_t size,
    bool host_visible = true,
    bool map = true
  ) -> std::unique_ptr<Buffer> = 0;

  template<typename T>
  auto CreateBufferWithData(
    Buffer::Usage usage,
    T const* data,
    size_t size,
    bool unmap = true
  ) -> std::unique_ptr<Buffer> {
    auto buffer = CreateBuffer(usage | Buffer::Usage::CopyDst, size);

    std::memcpy(buffer->Data(), data, size);
    buffer->Flush();

    if (unmap) {
      buffer->Unmap();
    }

    return buffer;
  }

  template<typename T>
  auto CreateBufferWithData(
    Buffer::Usage usage,
    ArrayView<T> const& data,
    bool unmap = true
  ) -> std::unique_ptr<Buffer> {
    return CreateBufferWithData(usage, data.data(), data.size() * sizeof(T), unmap);
  }

  template<typename T>
  auto CreateBufferWithData(
    Buffer::Usage usage,
    std::vector<T> const& data,
    bool unmap = true
  ) -> std::unique_ptr<Buffer> {
    return CreateBufferWithData(usage, data.data(), data.size() * sizeof(T), unmap);
  }

  virtual auto CreateShaderModule(
    u32 const* spirv,
    size_t size
  ) -> std::unique_ptr<ShaderModule> = 0;

  virtual auto CreateTexture2DFromSwapchainImage(
    u32 width,
    u32 height,
    GPUTexture::Format format,
    void* image_handle
  ) -> std::unique_ptr<GPUTexture> = 0;

  virtual auto CreateRenderTarget(
    std::vector<GPUTexture*> const& color_attachments,
    GPUTexture* depth_stencil_attachment = nullptr
  ) -> std::unique_ptr<RenderTarget> = 0;
};

} // namespace Aura
