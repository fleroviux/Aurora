/*
 * Copyright (C) 2022 fleroviux
 */

#pragma once

#include <aurora/gal/backend/vulkan.hpp>
#include <aurora/log.hpp>

namespace Aura {

struct VulkanBindGroup final : BindGroup {
  VulkanBindGroup(
    VkDevice device,
    VkDescriptorPool descriptor_pool,
    VkDescriptorSetLayout layout
  )   : device_(device), descriptor_pool_(descriptor_pool) {
    auto info = VkDescriptorSetAllocateInfo{
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      .pNext = nullptr,
      .descriptorPool = descriptor_pool,
      .descriptorSetCount = 1,
      .pSetLayouts = &layout
    };

    if (vkAllocateDescriptorSets(device, &info, &descriptor_set_) != VK_SUCCESS) {
      Assert(false, "VulkanBindGroup: failed to allocate descriptor set");
    }
  }

 ~VulkanBindGroup() override {
    vkFreeDescriptorSets(device_, descriptor_pool_, 1, &descriptor_set_);
  }

  auto Handle() -> void* override {
    return (void*)descriptor_set_;
  }

  void Bind(
    u32 binding,
    AnyPtr<Buffer> buffer,
    BindGroupLayout::Entry::Type type
  ) override {
    auto buffer_info = VkDescriptorBufferInfo{
      .buffer = (VkBuffer)buffer->Handle(),
      .offset = 0,
      .range = VK_WHOLE_SIZE
    };

    auto write_descriptor_set = VkWriteDescriptorSet{
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .pNext = nullptr,
      .dstSet = descriptor_set_,
      .dstBinding = binding,
      .dstArrayElement = 0,
      .descriptorCount = 1,
      .descriptorType = (VkDescriptorType)type,
      .pImageInfo = nullptr,
      .pBufferInfo = &buffer_info,
      .pTexelBufferView = nullptr
    };

    vkUpdateDescriptorSets(device_, 1, &write_descriptor_set, 0, nullptr);
  }

  void Bind(
    u32 binding,
    AnyPtr<GPUTexture> texture,
    AnyPtr<Sampler> sampler
  ) override {
    auto image_info = VkDescriptorImageInfo{
      .sampler = (VkSampler)sampler->Handle(),
      .imageView = (VkImageView)texture->handle(),
      .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL // TODO
    };

    auto write_descriptor_set = VkWriteDescriptorSet{
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .pNext = nullptr,
      .dstSet = descriptor_set_,
      .dstBinding = binding,
      .dstArrayElement = 0,
      .descriptorCount = 1,
      .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
      .pImageInfo = &image_info,
      .pBufferInfo = nullptr,
      .pTexelBufferView = nullptr
    };

    vkUpdateDescriptorSets(device_, 1, &write_descriptor_set, 0, nullptr);
  }

private:
  VkDevice device_;
  VkDescriptorPool descriptor_pool_;
  VkDescriptorSet descriptor_set_;
};

struct VulkanBindGroupLayout final : BindGroupLayout {
  VulkanBindGroupLayout(
    VkDevice device,
    VkDescriptorPool descriptor_pool,
    std::vector<BindGroupLayout::Entry> const& entries
  ) : device_(device), descriptor_pool_(descriptor_pool) {
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

  auto Instantiate() -> std::unique_ptr<BindGroup> override {
    return std::make_unique<VulkanBindGroup>(device_, descriptor_pool_, layout_);
  }

private:
  VkDevice device_;
  VkDescriptorPool descriptor_pool_;
  VkDescriptorSetLayout layout_;
};

} // namespace Aura
