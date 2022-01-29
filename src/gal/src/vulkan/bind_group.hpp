/*
 * Copyright (C) 2022 fleroviux
 */

#pragma once

#include <aurora/gal/backend/vulkan.hpp>
#include <aurora/log.hpp>

namespace Aura {

struct VulkanBindGroupLayout final : BindGroupLayout {
  VulkanBindGroupLayout(VkDevice device, std::vector<BindGroupLayout::Entry> const& entries) : device_(device) {
    auto bindings = std::vector<VkDescriptorSetLayoutBinding>{};

    for (auto const& entry : entries) {
      bindings.push_back({
        .binding = entry.binding,
        .descriptorType = (VkDescriptorType)entry.type,
        .descriptorCount = 1,
        .stageFlags = (VkShaderStageFlags)entry.stages,
        .pImmutableSamplers = nullptr
      });
    }

    auto info = VkDescriptorSetLayoutCreateInfo{
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .bindingCount = (u32)bindings.size(),
      .pBindings = bindings.data()
    };

    if (vkCreateDescriptorSetLayout(device, &info, nullptr, &layout_) != VK_SUCCESS) {
      Assert(false, "VulkanBindGroupLayout: failed to create descriptor set layout");
    }
  }

 ~VulkanBindGroupLayout() override {
    vkDestroyDescriptorSetLayout(device_, layout_, nullptr);
  }

  auto Handle() -> void* override {
   return (void*)layout_;
  }

private:
  VkDevice device_;
  VkDescriptorSetLayout layout_;
};

} // namespace Aura
