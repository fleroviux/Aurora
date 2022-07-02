
// Copyright (C) 2022 fleroviux. All rights reserved.

#pragma once

#include <aurora/gal/buffer.hpp>
#include <aurora/gal/sampler.hpp>
#include <aurora/gal/texture.hpp>
#include <aurora/any_ptr.hpp>
#include <aurora/integer.hpp>
#include <memory>

namespace Aura {

struct BindGroup;

struct BindGroupLayout {
  struct Entry {
    // subset of VkDescriptorType:
    // https://vulkan.lunarg.com/doc/view/latest/windows/apispec.html#VkDescriptorType
    enum class Type : u32 {
      ImageWithSampler = 1,
      UniformBuffer = 6
    };

    // subset of VkShaderStageFlagBits:
    // https://vulkan.lunarg.com/doc/view/latest/windows/apispec.html#VkShaderStageFlagBits
    enum class ShaderStage : u32 {
      Vertex = 0x00000001,
      Fragment = 0x00000010,
      All = 0x7FFFFFFF
    };

    u32 binding;
    Type type;
    ShaderStage stages = ShaderStage::All;
  };

  virtual ~BindGroupLayout() = default;

  virtual auto Handle() -> void* = 0;
  virtual auto Instantiate() -> std::unique_ptr<BindGroup> = 0;
};

constexpr auto operator|(
  BindGroupLayout::Entry::ShaderStage lhs,
  BindGroupLayout::Entry::ShaderStage rhs
) -> BindGroupLayout::Entry::ShaderStage {
  return (BindGroupLayout::Entry::ShaderStage)((u32)lhs | (u32)rhs);
}

struct BindGroup {
  virtual ~BindGroup() = default;

  virtual auto Handle() -> void* = 0;

  virtual void Bind(
    u32 binding,
    AnyPtr<Buffer> buffer,
    BindGroupLayout::Entry::Type type
  ) = 0;

  virtual void Bind(
    u32 binding,
    AnyPtr<Texture::View> texture_view,
    AnyPtr<Sampler> sampler,
    Texture::Layout layout
  ) = 0;

  void Bind(
    u32 binding,
    AnyPtr<Texture> texture,
    AnyPtr<Sampler> sampler,
    Texture::Layout layout
  ) {
    Bind(binding, texture->DefaultView(), sampler, layout);
  }
};

} // namespace Aura
