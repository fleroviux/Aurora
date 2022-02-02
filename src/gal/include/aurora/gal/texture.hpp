/*
 * Copyright (C) 2022 fleroviux
 */

#pragma once

#include <aurora/integer.hpp>

namespace Aura {

// TODO: make naming scheme consistent.
struct GPUTexture {
  // Subset of VkImageLayout:
  // https://vulkan.lunarg.com/doc/view/latest/windows/apispec.html#VkImageLayout
  enum class Layout {
    Undefined = 0,
    General = 1,
    ColorAttachment = 2,
    DepthStencilAttachment = 3,
    DepthStencilReadOnly = 4,
    ShaderReadOnly = 5,
    CopySrc = 6,
    CopyDst = 7
  };

  enum class Grade {
    _1D,
    _2D,
    _3D
  };

  // Subset of VkFormat:
  // https://vulkan.lunarg.com/doc/view/latest/windows/apispec.html#VkFormat
  // TODO: find a subset of formats that work on all targeted platforms.
  enum class Format {
    R8G8B8A8_SRGB = 43,
    B8G8R8A8_SRGB = 50,
    DEPTH_F32 = 126
  };

  // Subset of VkImageUsageFlagBits:
  // https://vulkan.lunarg.com/doc/view/latest/windows/apispec.html#VkImageUsageFlagBits
  enum class Usage : u32 {
    CopySrc = 0x00000001,
    CopyDst = 0x00000002,
    Sampled = 0x00000004,
    Storage = 0x00000008,
    ColorAttachment = 0x00000010,
    DepthStencilAttachment = 0x00000020
  };

  virtual ~GPUTexture() = default;

  virtual auto handle() -> void* = 0;
  virtual auto handle2() -> void* = 0;
  virtual auto grade() const -> Grade = 0;
  virtual auto format() const -> Format = 0;
  virtual auto usage() const -> Usage = 0;
  virtual auto width() const -> u32 = 0;
  virtual auto height() const -> u32 = 0;
  virtual auto depth() const -> u32 = 0;
};

constexpr auto operator|(
  GPUTexture::Usage lhs,
  GPUTexture::Usage rhs
) -> GPUTexture::Usage {
  return (GPUTexture::Usage)((u32)lhs | (u32)rhs);
}

} // namespace Aura
