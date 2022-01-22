/*
 * Copyright (C) 2022 fleroviux
 */

#pragma once

#include <aurora/gal/backend/vulkan.hpp>
#include <aurora/log.hpp>

#include "common.hpp"

namespace Aura {

struct VulkanBuffer final : Buffer {
  VulkanBuffer(
    VkPhysicalDevice physical_device,
    VkDevice device,
    Buffer::Usage usage,
    size_t size,
    bool host_visible,
    bool map
  )   : device(device), size(size), host_visible(host_visible) {
    auto buffer_info = VkBufferCreateInfo{
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .size = size,
      .usage = (VkBufferUsageFlags)usage,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .queueFamilyIndexCount = 0,
      .pQueueFamilyIndices = nullptr
    };

    if (vkCreateBuffer(device, &buffer_info, nullptr, &buffer) != VK_SUCCESS) {
      Assert(false, "VulkanBuffer: failed to create buffer, usage=0x{:08X}, size={}", (u32)usage, size);
    }

    auto memory_requirements = VkMemoryRequirements{};
    vkGetBufferMemoryRequirements(device, buffer, &memory_requirements);

    auto allocate_info = VkMemoryAllocateInfo{
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .pNext = nullptr,
      .allocationSize = memory_requirements.size,
      .memoryTypeIndex = vk_find_memory_type(
        physical_device,
        memory_requirements.memoryTypeBits,
        host_visible ? VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT : 0
      )
    };

    if (vkAllocateMemory(device, &allocate_info, nullptr, &memory) != VK_SUCCESS) {
      Assert(false, "VulkanBuffer: failed to allocate buffer memory, usage=0x{:08X}, size={}", (u32)usage, size);
    }

    vkBindBufferMemory(device, buffer, memory, 0);

    if (map) {
      Map();
    }
  }

 ~VulkanBuffer() override {
    Unmap();
    vkDestroyBuffer(device, buffer, nullptr);
    vkFreeMemory(device, memory, nullptr);
  }

  auto Handle() -> void* override {
    return (void*)buffer;
  }

  void Map() override {
    if (host_data == nullptr) {
      Assert(host_visible, "VulkanBuffer: attempted to map buffer which is not host visible");

      if (vkMapMemory(device, memory, 0, VK_WHOLE_SIZE, 0, &host_data) != VK_SUCCESS) {
        Assert(false, "VulkanBuffer: failed to map buffer to host memory, size={}", size);
      }
    }
  }

  void Unmap() override {
    if (host_data != nullptr) {
      vkUnmapMemory(device, memory);
      host_data = nullptr;
    }
  }

  auto Data() -> void* override {
    return host_data;
  }
  
  auto Size() const -> size_t override {
    return size;
  }

  void Flush() override {
    Flush(0, size);
  }

  void Flush(size_t offset, size_t size) override {
    auto range_end = offset + size;

    Assert(range_end <= this->size, "VulkanBuffer: out-of-bounds flush request, offset={}, size={}", offset, size);

    // TODO: Use offset and size instead of flushing the whole buffer.
    // This will require rounding them based on VkPhysicalDeviceLimits::nonCoherentAtomSize
    auto range = VkMappedMemoryRange{
      .sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
      .pNext = nullptr,
      .memory = memory,
      .offset = 0,
      .size = VK_WHOLE_SIZE
    };

    if (vkFlushMappedMemoryRanges(device, 1, &range) != VK_SUCCESS) {
      Assert(false, "VulkanBuffer: failed to flush range");
    }
  }

private:
  VkDevice device;
  VkBuffer buffer;
  VkDeviceMemory memory;
  size_t size;
  bool host_visible;
  void* host_data = nullptr;
};

} // namespace Aura
