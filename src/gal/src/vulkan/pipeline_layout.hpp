/*
 * Copyright (C) 2022 fleroviux
 */

#pragma once

#include <aurora/gal/backend/vulkan.hpp>
#include <aurora/log.hpp>
#include <vector>

namespace Aura {

struct VulkanPipelineLayout final : PipelineLayout {
  VulkanPipelineLayout(VkDevice device, std::vector<BindGroupLayout> const& bind_groups) : device_(device) {
    auto descriptor_set_layouts = std::vector<VkDescriptorSetLayout>{};

    for (auto const& bind_group : bind_groups) {
      auto bindings = std::vector<VkDescriptorSetLayoutBinding>{};

      for (auto const& bind_group_entry : bind_group) {
        bindings.push_back({
          .binding = bind_group_entry.binding,
          .descriptorType = (VkDescriptorType)bind_group_entry.type,
          .descriptorCount = 1,
          .stageFlags = (VkShaderStageFlags)bind_group_entry.stages,
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

      auto layout = VkDescriptorSetLayout{};

      if (vkCreateDescriptorSetLayout(device, &info, nullptr, &layout) != VK_SUCCESS) {
        Assert(false, "VulkanPipelineLayout: failed to create descriptor set layout");
      }

      descriptor_set_layouts.push_back(layout);
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
    // TODO: destroy descriptor set layouts
    vkDestroyPipelineLayout(device_, layout_, nullptr);
  }

  auto Handle() -> void* override {
    return (void*)layout_;
  }

private:
  VkDevice device_;
  VkPipelineLayout layout_;
};

} // namespace Aura
