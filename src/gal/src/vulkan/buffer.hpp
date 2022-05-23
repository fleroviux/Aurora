/*
 * Copyright (C) 2022 fleroviux
 */

#pragma once

#include <aurora/gal/backend/vulkan.hpp>
#include <aurora/log.hpp>
#include <vk_mem_alloc.h>

#include "common.hpp"
#include "command_buffer.hpp"

namespace Aura {

struct VulkanBuffer final : Buffer {
  VulkanBuffer(
    VmaAllocator allocator,
    VulkanCommandBuffer* transfer_cmd_buffer,
    Buffer::Usage usage,
    size_t size,
    bool host_visible,
    bool map
  )   : allocator(allocator), transfer_cmd_buffer(transfer_cmd_buffer), size(size), host_visible(false) {
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

    // TODO: skip staging buffer creation on devices with UMA (e.g. Apple M1 SoC)
    // TODO: create and destroy staging buffer on demand (when data is static/not dynamic)

    auto alloc_info = VmaAllocationCreateInfo{
      .usage = VMA_MEMORY_USAGE_GPU_ONLY
    };

    if (host_visible) {
      // force transfer destination bit when a staging buffer is used.
      // TODO: evaluate if this is actually a good idea.
      buffer_info.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;

      staging_buffer = std::make_unique<VulkanBuffer>(allocator, size);
    }

    if (vmaCreateBuffer(allocator, &buffer_info, &alloc_info, &buffer, &allocation, nullptr) != VK_SUCCESS) {
      Assert(false, "VulkanBuffer: failed to create buffer");
    }

    if (map) {
      Map();
    }
  }

  // TODO: make it clear what kind of buffer is created
  VulkanBuffer(
    VmaAllocator allocator,
    size_t size
  )   : allocator(allocator), size(size), host_visible(true) {
    auto buffer_info = VkBufferCreateInfo{
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .size = size,
      .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .queueFamilyIndexCount = 0,
      .pQueueFamilyIndices = nullptr
    };

    auto alloc_info = VmaAllocationCreateInfo{
      .usage = VMA_MEMORY_USAGE_CPU_ONLY
    };

    vmaCreateBuffer(allocator, &buffer_info, &alloc_info, &buffer, &allocation, nullptr);
  }

 ~VulkanBuffer() override {
    Unmap();
    vmaDestroyBuffer(allocator, buffer, allocation);
  }

  auto Handle() -> void* override {
    return (void*)buffer;
  }

  void Map() override {
    if (staging_buffer) {
      staging_buffer->Map();
      host_data = staging_buffer->Data();
      return;
    }

    if (host_data == nullptr) {
      Assert(host_visible, "VulkanBuffer: attempted to map buffer which is not host visible");

      if (vmaMapMemory(allocator, allocation, &host_data) != VK_SUCCESS) {
        Assert(false, "VulkanBuffer: failed to map buffer to host memory, size={}", size);
      }
    }
  }

  void Unmap() override {
    if (staging_buffer) {
      staging_buffer->Unmap();
      host_data = nullptr;
      return;
    }

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
    if (staging_buffer) {
      // TODO: implement buffer copy as a method on CommandBuffer
      auto region = VkBufferCopy{
        .srcOffset = offset,
        .dstOffset = offset,
        .size = size
      };

      staging_buffer->Flush(offset, size);

      vkCmdCopyBuffer((VkCommandBuffer)transfer_cmd_buffer->Handle(), (VkBuffer)staging_buffer->Handle(), buffer, 1, &region);
      return;
    }

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
  VulkanCommandBuffer* transfer_cmd_buffer;
  size_t size;
  bool host_visible;
  void* host_data = nullptr;
  std::unique_ptr<VulkanBuffer> staging_buffer;
};

} // namespace Aura
