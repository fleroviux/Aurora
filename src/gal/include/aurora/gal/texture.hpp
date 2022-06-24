/*
 * Copyright (C) 2022 fleroviux
 */

#pragma once

#include <aurora/any_ptr.hpp>
#include <aurora/integer.hpp>

namespace Aura {

// TODO: make naming scheme consistent.
struct GPUTexture {
  // subset of VkImageLayout:
  // https://vulkan.lunarg.com/doc/view/latest/windows/apispec.html#VkImageLayout
  enum class Layout {
    Undefined = 0,
    General = 1,
    ColorAttachment = 2,
    DepthStencilAttachment = 3,
    DepthStencilReadOnly = 4,
    ShaderReadOnly = 5,
    CopySrc = 6,
    CopyDst = 7,

    // VK_KHR_swapchain
    PresentSrc = 1000001002,

    // Vulkan 1.2
    DepthAttachment = 1000241000,
    DepthReadOnly = 1000241001,
    StencilAttachment = 1000241002,
    StencilReadOnly = 1000241003,
  };

  enum class Grade {
    _1D,
    _2D,
    _3D
  };

  // subset of VkFormat:
  // https://vulkan.lunarg.com/doc/view/latest/windows/apispec.html#VkFormat
  // TODO: find a subset of formats that work on all targeted platforms.
  enum class Format {
    R8G8B8A8_SRGB = 43,
    B8G8R8A8_SRGB = 50,
    DEPTH_F32 = 126
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

  // subset of VkImageAspectFlags:
  // https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/VkImageAspectFlagBits.html
  enum class Aspect : u32 {
    Color = 0x00000001,
    Depth = 0x00000002,
    Stencil = 0x00000004,
    Metadata = 0x00000008
  };

  struct SubresourceRange {
    Aspect aspect = Aspect::Color;
    u32 mip_base = 0;
    u32 mip_count = 1;
    u32 layer_base = 0;
    u32 layer_count = 1;
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
  virtual auto layers() const -> u32 = 0;
  virtual auto mip_levels() const -> u32 = 0;
};

constexpr auto operator|(GPUTexture::Usage lhs, GPUTexture::Usage rhs) -> GPUTexture::Usage {
  return static_cast<GPUTexture::Usage>(static_cast<u32>(lhs) | static_cast<u32>(rhs));
}

constexpr auto operator|(GPUTexture::Aspect lhs, GPUTexture::Aspect rhs) -> GPUTexture::Aspect {
  return static_cast<GPUTexture::Aspect>(static_cast<u32>(lhs) | static_cast<u32>(rhs));
}

} // namespace Aura
