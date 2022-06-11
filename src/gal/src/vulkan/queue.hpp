/*
 * Copyright (C) 2022 fleroviux
 */

#pragma once

#include <aurora/gal/backend/vulkan.hpp>

namespace Aura {

struct VulkanQueue final : Queue {
  VulkanQueue(VkQueue queue) : queue(queue) {}

  auto Handle() -> void* override {
    return (void*)queue;
  }

  void Submit(ArrayView<CommandBuffer*> buffers, AnyPtr<Fence> fence) override {
    VkCommandBuffer handles[buffers.size()];

    for (size_t i = 0; i < buffers.size(); i++) {
      handles[i] = (VkCommandBuffer)buffers[i]->Handle();
    }

    auto submit = VkSubmitInfo{
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .pNext = nullptr,
      .waitSemaphoreCount = 0,
      .pWaitSemaphores = nullptr,
      .pWaitDstStageMask = nullptr,
      .commandBufferCount = (u32)buffers.size(),
      .pCommandBuffers = handles,
      .signalSemaphoreCount = 0,
      .pSignalSemaphores = nullptr
    };

    vkQueueSubmit(queue, 1, &submit, (VkFence)fence->Handle());
  }

private:
  VkQueue queue;
};

} // namespace Aura
