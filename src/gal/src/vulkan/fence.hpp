
// Copyright (C) 2022 fleroviux. All rights reserved.

#pragma once

#include <aurora/gal/backend/vulkan.hpp>
#include <aurora/log.hpp>

namespace Aura {

struct VulkanFence final : Fence {
  VulkanFence(VkDevice device) : device(device) {
    auto info = VkFenceCreateInfo{
      .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0
    };

    if (vkCreateFence(device, &info, nullptr, &fence) != VK_SUCCESS) {
      Assert(false, "VulkanFence: failed to create fence :(");
    }
  }

 ~VulkanFence() override {
    vkDestroyFence(device, fence, nullptr);
  }

  auto Handle() -> void* override {
    return (void*)fence;
  }

  void Reset() override {
    vkResetFences(device, 1, &fence);
  }

  void Wait(u64 timeout_ns) override {
    vkWaitForFences(device, 1, &fence, VK_FALSE, timeout_ns);
  }

private:
  VkDevice device;
  VkFence fence;
};

} // namespace Aura
