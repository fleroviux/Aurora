/*
 * Copyright (C) 2022 fleroviux
 */

#pragma once

#include <aurora/gal/backend/vulkan.hpp>
#include <aurora/log.hpp>

namespace Aura {

struct VulkanCommandPool final : CommandPool {
  VulkanCommandPool(VkDevice device, u32 queue_family, Usage usage) : device(device) {
    auto info = VkCommandPoolCreateInfo{
      .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .pNext = nullptr,
      .flags = (VkCommandPoolCreateFlags)usage,
      .queueFamilyIndex = queue_family
    };

    if (vkCreateCommandPool(device, &info, nullptr, &pool) != VK_SUCCESS) {
      Assert(false, "Vulkan: failed to create a command pool");
    }
  }

 ~VulkanCommandPool() override {
    vkDestroyCommandPool(device, pool, nullptr);
  }

  auto Handle() -> void* override {
    return (void*)pool;
  }

private:
  VkDevice device;
  VkCommandPool pool;
};

} // namespace Aura
