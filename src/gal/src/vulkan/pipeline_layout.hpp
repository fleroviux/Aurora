
// Copyright (C) 2022 fleroviux. All rights reserved.

#pragma once

#include <aurora/gal/backend/vulkan.hpp>

namespace Aura {

struct VulkanPipelineLayout final : PipelineLayout {
  VulkanPipelineLayout(
    VkDevice device,
    std::vector<std::shared_ptr<BindGroupLayout>> const& bind_groups
  )   : device_(device), bind_groups_(bind_groups) {
    auto descriptor_set_layouts = std::vector<VkDescriptorSetLayout>{};

    for (auto& bind_group : bind_groups) {
      descriptor_set_layouts.push_back((VkDescriptorSetLayout)bind_group->Handle());
    }

    auto info = VkPipelineLayoutCreateInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .setLayoutCount = (u32)descriptor_set_layouts.size(),
      .pSetLayouts = descriptor_set_layouts.data(),
      .pushConstantRangeCount = 0,
      .pPushConstantRanges = nullptr
    };

    if (vkCreatePipelineLayout(device, &info, nullptr, &layout_) != VK_SUCCESS) {
      Assert(false, "VulkanPipelineLayout: failed to create pipeline layout");
    }
  }

 ~VulkanPipelineLayout() override {
    vkDestroyPipelineLayout(device_, layout_, nullptr);
  }

  auto Handle() -> void* override {
    return (void*)layout_;
  }

private:
  VkDevice device_;
  VkPipelineLayout layout_;
  std::vector<std::shared_ptr<BindGroupLayout>> bind_groups_;
};

} // namespace Aura
