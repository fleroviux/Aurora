/*
 * Copyright (C) 2022 fleroviux
 */

#pragma once

namespace Aura {

// TODO: make naming scheme consistent.
struct GPUTexture {
  // subset of VkFormat:
  // https://vulkan.lunarg.com/doc/view/latest/windows/apispec.html#VkFormat
  // TODO: find a subset of formats that work on all targeted platforms.
  enum class Format {
    B8G8R8B8_SRGB = 50
  };

  // subset of VkImageUsageFlagBits:
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
};

constexpr auto operator|(
  GPUTexture::Usage lhs,
  GPUTexture::Usage rhs
) -> GPUTexture::Usage {
  return (GPUTexture::Usage)((u32)lhs | (u32)rhs);
}

} // namespace Aura
