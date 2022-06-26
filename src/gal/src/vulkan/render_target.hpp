
// Copyright (C) 2022 fleroviux. All rights reserved.

#pragma once

#include <aurora/gal/backend/vulkan.hpp>
#include <aurora/log.hpp>

#include "render_pass.hpp"

namespace Aura {

struct VulkanRenderTarget final : RenderTarget {
  VulkanRenderTarget(
    VkDevice device,
    std::vector<std::shared_ptr<Texture>> const& color_attachments,
    std::shared_ptr<Texture> depth_stencil_attachment
  )   : device(device)
      , color_attachments(color_attachments)
      , depth_stencil_attachment(depth_stencil_attachment) {
    auto image_views = std::vector<VkImageView>{};

    Texture* first_attachment = nullptr;

    if (color_attachments.size() > 0) {
      first_attachment = color_attachments[0].get();
    } else {
      Assert(!!depth_stencil_attachment, "VulkanRenderTarget: cannot have render target with zero attachments");

      first_attachment = depth_stencil_attachment.get();
    }

    width_ = first_attachment->GetWidth();
    height_ = first_attachment->GetHeight();

    render_pass = CreateRenderPass();

    for (auto& texture : color_attachments) {
      image_views.push_back((VkImageView)texture->DefaultView()->Handle());
    }

    if (depth_stencil_attachment) {
      image_views.push_back((VkImageView)depth_stencil_attachment->DefaultView()->Handle());
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

private:
  auto CreateRenderPass() -> std::unique_ptr<RenderPass> {
    auto color_attachment_count = color_attachments.size();
    auto render_pass_builder = VulkanRenderPassBuilder{device};

    for (size_t i = 0; i < color_attachment_count; i++) {
      render_pass_builder.SetColorAttachment(i, {
        color_attachments[i]->GetFormat(),
        Texture::Layout::Undefined,
        Texture::Layout::ColorAttachment
      });
    }

    if (depth_stencil_attachment) {
      render_pass_builder.SetDepthAttachment({
        depth_stencil_attachment->GetFormat(),
        Texture::Layout::Undefined,
        Texture::Layout::DepthStencilAttachment
      });
    }

    return render_pass_builder.Build();
  }

  VkDevice device;
  VkFramebuffer framebuffer;
  std::unique_ptr<RenderPass> render_pass;
  std::vector<std::shared_ptr<Texture>> color_attachments;
  std::shared_ptr<Texture> depth_stencil_attachment;
  u32 width_;
  u32 height_;
};

} // namespace Aura
