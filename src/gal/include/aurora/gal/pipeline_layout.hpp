/*
 * Copyright (C) 2022 fleroviux
 */

#pragma once

namespace Aura {

struct BindGroupLayoutEntry {
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

 constexpr auto operator|(
  BindGroupLayoutEntry::ShaderStage lhs,
  BindGroupLayoutEntry::ShaderStage rhs
) -> BindGroupLayoutEntry::ShaderStage {
  return (BindGroupLayoutEntry::ShaderStage)((u32)lhs | (u32)rhs);
}

struct PipelineLayout {
  virtual ~PipelineLayout() = default;

  virtual auto Handle() -> void* = 0;
};

} // namespace Aura
