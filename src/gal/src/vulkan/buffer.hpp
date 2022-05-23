/*
 * Copyright (C) 2022 fleroviux
 */

#pragma once

#include <aurora/gal/backend/vulkan.hpp>
#include <aurora/log.hpp>
#include <vk_mem_alloc.h>

#include "common.hpp"

namespace Aura {

struct VulkanBuffer final : Buffer {
  VulkanBuffer(
    VmaAllocator allocator,
    Buffer::Usage usage,
    size_t size,
    bool host_visible,
    bool map
  )   : allocator(allocator), size(size), host_visible(host_visible) {
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

    // TODO: initialize this properly and account for host_visible flag.
    auto alloc_info = VmaAllocationCreateInfo{
      .usage = VMA_MEMORY_USAGE_CPU_ONLY
    };

    vmaCreateBuffer(allocator, &buffer_info, &alloc_info, &buffer, &allocation, nullptr);

    if (map) {
      Map();
    }
  }

 ~VulkanBuffer() override {
    Unmap();
    vmaDestroyBuffer(allocator, buffer, allocation);
  }

  auto Handle() -> void* override {
    return (void*)buffer;
  }

  void Map() override {
    if (host_data == nullptr) {
      Assert(host_visible, "VulkanBuffer: attempted to map buffer which is not host visible");

      if (vmaMapMemory(allocator, allocation, &host_data) != VK_SUCCESS) {
        Assert(false, "VulkanBuffer: failed to map buffer to host memory, size={}", size);
      }
    }
  }

  void Unmap() override {
    if (host_data != nullptr) {
      vmaUnmapMemory(allocator, allocation);
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

    if (vmaFlushAllocation(allocator, allocation, offset, size) != VK_SUCCESS) {
      Assert(false, "VulkanBuffer: failed to flush range");
    }
  }

private:
  VkBuffer buffer;
  VmaAllocator allocator;
  VmaAllocation allocation;
  size_t size;
  bool host_visible;
  void* host_data = nullptr;
};

} // namespace Aura
