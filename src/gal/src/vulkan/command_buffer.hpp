/*
 * Copyright (C) 2022 fleroviux
 */

#pragma once

#include <aurora/gal/backend/vulkan.hpp>
#include <aurora/log.hpp>
#include <memory>

#include "render_pass.hpp"

namespace Aura {

struct VulkanCommandBuffer final : CommandBuffer {
  VulkanCommandBuffer(VkDevice device, std::shared_ptr<CommandPool> pool)
      : device(device)
      , pool(pool) {
    auto info = VkCommandBufferAllocateInfo{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .pNext = nullptr,
      .commandPool = (VkCommandPool)pool->Handle(),
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = 1
    };

    if (vkAllocateCommandBuffers(device, &info, &buffer) != VK_SUCCESS) {
      Assert(false, "Vulkan: failed to allocate command buffer");
    }
  }

 ~VulkanCommandBuffer() override {
    vkFreeCommandBuffers(device, (VkCommandPool)pool->Handle(), 1, &buffer);
  }

  auto Handle() -> void* override {
    return (void*)buffer;
  }

  void Begin(OneTimeSubmit one_time_submit) override {
    const auto begin_info = VkCommandBufferBeginInfo{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .pNext = nullptr,
      .flags = (VkCommandBufferUsageFlags)one_time_submit,
      .pInheritanceInfo = nullptr
    };

    vkBeginCommandBuffer(buffer, &begin_info);
  }

  void End() override {
    vkEndCommandBuffer(buffer);
  }

  void BeginRenderPass(
    AnyPtr<RenderTarget> render_target,
    AnyPtr<RenderPass> render_pass
  ) override {
    auto vk_render_pass = (VulkanRenderPass*)render_pass.get();
    auto& clear_values = vk_render_pass->GetClearValues();

    auto info = VkRenderPassBeginInfo{
      .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
      .pNext = nullptr,
      .renderPass = vk_render_pass->Handle(),
      .framebuffer = (VkFramebuffer)render_target->handle(),
      .renderArea = VkRect2D{
        .offset = VkOffset2D{
          .x = 0,
          .y = 0
        },
        .extent = VkExtent2D{
          .width = render_target->width(),
          .height = render_target->height()
        }
      },
      .clearValueCount = (u32)clear_values.size(),
      .pClearValues = clear_values.data()
    };

    vkCmdBeginRenderPass(buffer, &info, VK_SUBPASS_CONTENTS_INLINE);
  }

  void EndRenderPass() override {
    vkCmdEndRenderPass(buffer);
  }

private:
  VkDevice device;
  VkCommandBuffer buffer;
  std::shared_ptr<CommandPool> pool;
};

} // namespace Aura
