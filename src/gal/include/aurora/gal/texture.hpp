
// Copyright (C) 2022 fleroviux. All rights reserved.

#pragma once

#include <aurora/any_ptr.hpp>
#include <aurora/integer.hpp>

namespace Aura {

// equivalent to VkComponentSwizzle:
// https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/VkComponentSwizzle.html 
enum class ComponentSwizzle {
  Identity = 0,
  Zero = 1,
  One = 2,
  R = 3,
  G = 4,
  B = 5,
  A = 6
};

struct ComponentMapping {
  ComponentSwizzle r = ComponentSwizzle::Identity;
  ComponentSwizzle g = ComponentSwizzle::Identity;
  ComponentSwizzle b = ComponentSwizzle::Identity;
  ComponentSwizzle a = ComponentSwizzle::Identity;
};

struct Texture {
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

  // equivalent to VkImageType:
  // https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/VkImageType.html
  enum class Grade {
    _1D = 0,
    _2D = 1,
    _3D = 2
  };

  // subset of VkFormat:
  // https://vulkan.lunarg.com/doc/view/latest/windows/apispec.html#VkFormat
  // TODO: find a subset of formats that work on all targeted platforms.
  enum class Format {
    R8G8B8A8_SRGB = 43,
    B8G8R8A8_SRGB = 50,
    R32G32B32A32_SFLOAT = 109,
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
    u32 base_mip = 0;
    u32 mip_count = 1;
    u32 base_layer = 0;
    u32 layer_count = 1;
  };

  struct View {
    // equivalent to VkImageViewType:
    // https://www.khronos.org/registry/vulkan/specs/1.3-extensions/html/vkspec.html#VkImageViewType
    enum class Type {
      _1D = 0,
      _2D = 1,
      _3D = 2,
      Cube = 3,
      _1D_Array = 4,
      _2D_Array = 5,
      CubeArray = 6
    };

    virtual ~View() = default;

    virtual auto Handle() -> void* = 0;
    virtual auto GetType() const -> Type = 0;
    virtual auto GetFormat() const -> Texture::Format = 0;
    virtual auto GetAspect() const -> Texture::Aspect = 0;
    virtual auto GetBaseMip() const -> u32 = 0;
    virtual auto GetMipCount() const -> u32 = 0;
    virtual auto GetBaseLayer() const -> u32 = 0;
    virtual auto GetLayerCount() const -> u32 = 0;
    virtual auto GetSubresourceRange() const -> Texture::SubresourceRange const& = 0;
    virtual auto GetComponentMapping() const -> ComponentMapping const& = 0;
  };

  virtual ~Texture() = default;

  virtual auto Handle() -> void* = 0;
  virtual auto GetGrade() const -> Grade = 0;
  virtual auto GetFormat() const -> Format = 0;
  virtual auto GetUsage() const -> Usage = 0;
  virtual auto GetWidth() const -> u32 = 0;
  virtual auto GetHeight() const -> u32 = 0;
  virtual auto GetDepth() const -> u32 = 0;
  virtual auto GetLayerCount() const -> u32 = 0;
  virtual auto GetMipCount() const -> u32 = 0;

  virtual auto DefaultSubresourceRange() const -> SubresourceRange = 0;

  virtual auto DefaultView() const -> View const* = 0;
  virtual auto DefaultView() -> View* = 0;

  virtual auto CreateView(
    View::Type type,
    Format format,
    SubresourceRange const& range,
    ComponentMapping const& mapping = {}
  ) -> std::unique_ptr<View> = 0;
};

constexpr auto operator|(Texture::Usage lhs, Texture::Usage rhs) -> Texture::Usage {
  return static_cast<Texture::Usage>(static_cast<u32>(lhs) | static_cast<u32>(rhs));
}

constexpr auto operator|(Texture::Aspect lhs, Texture::Aspect rhs) -> Texture::Aspect {
  return static_cast<Texture::Aspect>(static_cast<u32>(lhs) | static_cast<u32>(rhs));
}

} // namespace Aura
