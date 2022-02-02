/*
 * Copyright (C) 2022 fleroviux
 */

#pragma once

#include <aurora/gal/backend/vulkan.hpp>
#include <aurora/log.hpp>

namespace Aura {

struct VulkanRenderPass final : RenderPass {
  VulkanRenderPass(
    VkDevice device,
    std::vector<std::shared_ptr<GPUTexture>> const& color_attachments,
    std::vector<RenderPass::Descriptor> const& color_descriptors,
    std::shared_ptr<GPUTexture> depth_stencil_attachment,
    RenderPass::Descriptor depth_stencil_descriptor
  )   : device_(device) {
    auto attachments = std::vector<VkAttachmentDescription>{};
    auto references = std::vector<VkAttachmentReference>{};

    // TODO: fill out structures according to descriptors

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

      references.push_back(VkAttachmentReference{
        .attachment = (u32)index,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
      });
    }

    auto depth_reference = VkAttachmentReference{
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
    }

    auto subpass_info = VkSubpassDescription{
      .flags = 0,
      .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
      .inputAttachmentCount = 0,
      .pInputAttachments = nullptr,
      .colorAttachmentCount = (u32)references.size(),
      .pColorAttachments = references.data(),
      .pResolveAttachments = nullptr,
      .pDepthStencilAttachment = nullptr,
      .preserveAttachmentCount = 0,
      .pPreserveAttachments = nullptr
    };

    if (depth_stencil_attachment) {
      subpass_info.pDepthStencilAttachment = &depth_reference;
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

    if (vkCreateRenderPass(device_, &pass_info, nullptr, &render_pass_) != VK_SUCCESS) {
      Assert(false, "VulkanRenderPass: failed to create render pass");
    }
  }

 ~VulkanRenderPass() {
    vkDestroyRenderPass(device_, render_pass_, nullptr);
  }

  auto Handle() -> VkRenderPass { return render_pass_; }

  void SetClearColor(float r, float g, float b, float a) override {
    // TODO
  }

  void SetClearDepth(float depth) override {
    // TODO
  }

  void SetClearStencil(u32 stencil) override {
    // TODO
  }

private:
  VkDevice device_;
  VkRenderPass render_pass_;
};

} // namespace Aura
