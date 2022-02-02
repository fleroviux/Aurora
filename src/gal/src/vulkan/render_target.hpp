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
    std::vector<std::shared_ptr<GPUTexture>> const& color_attachments,
    std::shared_ptr<GPUTexture> depth_stencil_attachment
  )   : device(device)
      , color_attachments(color_attachments)
      , depth_stencil_attachment(depth_stencil_attachment) {
    auto attachments = std::vector<VkAttachmentDescription>{};
    auto attachment_refs = std::vector<VkAttachmentReference>{};
    auto attachment_handles = std::vector<VkImageView>{};

    for (auto& texture : color_attachments) {
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

    auto depth_attachment_ref = VkAttachmentReference{
      .attachment = (u32)attachments.size(),
      .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };

    if (depth_stencil_attachment) {
      attachments.push_back(VkAttachmentDescription{
        .flags = 0,
        .format = (VkFormat)depth_stencil_attachment->format(),
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
      });

      attachment_handles.push_back((VkImageView)depth_stencil_attachment->handle());
    }

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

    if (depth_stencil_attachment) {
      subpass_info.pDepthStencilAttachment = &depth_attachment_ref;
    }

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

    GPUTexture* first_attachment = nullptr;

    if (color_attachments.size() > 0) {
      first_attachment = color_attachments[0].get();
    } else {
      Assert(!!depth_stencil_attachment, "VulkanRenderTarget: cannot have render target with zero attachments");

      first_attachment = depth_stencil_attachment.get();
    }

    auto frame_buffer_info = VkFramebufferCreateInfo{
      .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .renderPass = render_pass,
      .attachmentCount = (u32)attachment_handles.size(),
      .pAttachments = attachment_handles.data(),
      .width = first_attachment->width(),
      .height = first_attachment->height(),
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
  std::vector<std::shared_ptr<GPUTexture>> color_attachments;
  std::shared_ptr<GPUTexture> depth_stencil_attachment;
};

} // namespace Aura
