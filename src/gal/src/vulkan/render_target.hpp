/*
 * Copyright (C) 2022 fleroviux
 */

#pragma once

#include <aurora/gal/backend/vulkan.hpp>
#include <aurora/log.hpp>

namespace Aura {

struct VulkanRenderTarget final : RenderTarget {
  VulkanRenderTarget(
    VkDevice device,
    std::vector<GPUTexture*> const& color_attachments,
    GPUTexture* depth_stencil_attachment
  )   : device(device) {
    auto attachments = std::vector<VkAttachmentDescription>{};
    auto attachment_refs = std::vector<VkAttachmentReference>{};
    auto attachment_handles = std::vector<VkImageView>{};

    for (GPUTexture* texture : color_attachments) {
      auto index = attachments.size();

      attachments.push_back(VkAttachmentDescription{
        .flags = 0,
        .format = (VkFormat)texture->format(),
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
      });

      attachment_refs.push_back(VkAttachmentReference{
        .attachment = (u32)index,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
      });

      attachment_handles.push_back((VkImageView)texture->handle());
    }

    // TODO: depth-stencil attachment

    auto subpass_info = VkSubpassDescription{
      .flags = 0,
      .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
      .inputAttachmentCount = 0,
      .pInputAttachments = nullptr,
      .colorAttachmentCount = (u32)attachment_refs.size(),
      .pColorAttachments = attachment_refs.data(),
      .pResolveAttachments = nullptr,
      .pDepthStencilAttachment = nullptr,
      .preserveAttachmentCount = 0,
      .pPreserveAttachments = nullptr
    };

    auto pass_info = VkRenderPassCreateInfo{
      .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .attachmentCount = (u32)attachments.size(),
      .pAttachments = attachments.data(),
      .subpassCount = 1,
      .pSubpasses = &subpass_info,
      .dependencyCount = 0,
      .pDependencies = nullptr
    };

    if (vkCreateRenderPass(device, &pass_info, nullptr, &render_pass) != VK_SUCCESS) {
      Assert(false, "VulkanRenderTarget: failed to create render pass");
    }

    // TODO: this breaks if we only have a depth attachment!
    auto frame_buffer_info = VkFramebufferCreateInfo{
      .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .renderPass = render_pass,
      .attachmentCount = (u32)attachment_handles.size(),
      .pAttachments = attachment_handles.data(),
      .width = color_attachments[0]->width(),
      .height = color_attachments[0]->height(),
      .layers = 1
    };

    if (vkCreateFramebuffer(device, &frame_buffer_info, nullptr, &framebuffer) != VK_SUCCESS) {
      Assert(false, "VulkanRenderTarget: failed to create framebuffer");
    }
  }

 ~VulkanRenderTarget() override {
    vkDestroyRenderPass(device, render_pass, nullptr);
    vkDestroyFramebuffer(device, framebuffer, nullptr);
  }

  auto handle() -> void* override { return (void*)framebuffer; }

private:
  VkDevice device;
  VkFramebuffer framebuffer;
  VkRenderPass render_pass;
};

} // namespace Aura
