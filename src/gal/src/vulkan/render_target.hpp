/*
 * Copyright (C) 2022 fleroviux
 */

#pragma once

#include <aurora/gal/backend/vulkan.hpp>
#include <aurora/log.hpp>

#include "render_pass.hpp"

namespace Aura {

struct VulkanRenderTarget final : RenderTarget {
  VulkanRenderTarget(
    VkDevice device,
    std::vector<std::shared_ptr<GPUTexture>> const& color_attachments,
    std::shared_ptr<GPUTexture> depth_stencil_attachment
  )   : device(device)
      , color_attachments(color_attachments)
      , depth_stencil_attachment(depth_stencil_attachment) {
    auto image_views = std::vector<VkImageView>{};

    GPUTexture* first_attachment = nullptr;

    if (color_attachments.size() > 0) {
      first_attachment = color_attachments[0].get();
    } else {
      Assert(!!depth_stencil_attachment, "VulkanRenderTarget: cannot have render target with zero attachments");

      first_attachment = depth_stencil_attachment.get();
    }

    width_ = first_attachment->width();
    height_ = first_attachment->height();

    render_pass = CreateRenderPass();

    for (auto& texture : color_attachments) {
      image_views.push_back((VkImageView)texture->handle());
    }

    if (depth_stencil_attachment) {
      image_views.push_back((VkImageView)depth_stencil_attachment->handle());
    }

    auto info = VkFramebufferCreateInfo{
      .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .renderPass = ((VulkanRenderPass*)render_pass.get())->Handle(),
      .attachmentCount = (u32)image_views.size(),
      .pAttachments = image_views.data(),
      .width = width(),
      .height = height(),
      .layers = 1
    };

    if (vkCreateFramebuffer(device, &info, nullptr, &framebuffer) != VK_SUCCESS) {
      Assert(false, "VulkanRenderTarget: failed to create framebuffer");
    }
  }

 ~VulkanRenderTarget() override {
    vkDestroyFramebuffer(device, framebuffer, nullptr);
  }

  auto handle() -> void* override { return (void*)framebuffer; }
  auto width() -> u32 override { return width_; }
  auto height() -> u32 override { return height_; }

  auto CreateRenderPass(
    std::vector<RenderPass::Descriptor> const& color_descriptors = {{}},
    RenderPass::Descriptor depth_stencil_descriptor = {}
  ) -> std::unique_ptr<RenderPass> override {
    return std::make_unique<VulkanRenderPass>(
      device,
      color_attachments,
      color_descriptors,
      depth_stencil_attachment,
      depth_stencil_descriptor
    );
  }

private:
  VkDevice device;
  VkFramebuffer framebuffer;
  std::unique_ptr<RenderPass> render_pass;
  std::vector<std::shared_ptr<GPUTexture>> color_attachments;
  std::shared_ptr<GPUTexture> depth_stencil_attachment;
  u32 width_;
  u32 height_;
};

} // namespace Aura
