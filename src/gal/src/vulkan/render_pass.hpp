
// Copyright (C) 2022 fleroviux. All rights reserved.

#pragma once

#include <algorithm>
#include <aurora/gal/backend/vulkan.hpp>
#include <aurora/log.hpp>
#include <vector>

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

    Assert(color_attachments.size() == 0 || color_descriptors.size() > 0,
      "VulkanRenderPass: must have at least one color descriptor when using a color attachment");

    for (auto& texture : color_attachments) {
      auto index = attachments.size();
      auto& descriptor = color_descriptors[std::min(index, color_descriptors.size() - 1)];

      attachments.push_back(VkAttachmentDescription{
        .flags = 0,
        .format = (VkFormat)texture->format(),
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = (VkAttachmentLoadOp)descriptor.load_op,
        .storeOp = (VkAttachmentStoreOp)descriptor.store_op,
        .stencilLoadOp = (VkAttachmentLoadOp)descriptor.stencil_load_op,
        .stencilStoreOp = (VkAttachmentStoreOp)descriptor.stencil_store_op,
        .initialLayout = (VkImageLayout)descriptor.layout_src,
        .finalLayout = (VkImageLayout)descriptor.layout_dst
      });

      references.push_back(VkAttachmentReference{
        .attachment = (u32)index,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
      });

      clear_values_.push_back(VkClearValue{
        .color = VkClearColorValue{
          .float32 = {0, 0, 0, 1}
        }
      });
    }

    color_attachment_count_ = color_attachments.size();

    auto depth_reference = VkAttachmentReference{
      .attachment = (u32)attachments.size(),
      .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };

    if (depth_stencil_attachment) {
      auto layout_src = depth_stencil_descriptor.layout_src;
      auto layout_dst = depth_stencil_descriptor.layout_dst;

      if (layout_src == GPUTexture::Layout::ColorAttachment) {
        layout_src = GPUTexture::Layout::DepthStencilAttachment;
      }

      if (layout_dst == GPUTexture::Layout::ColorAttachment) {
        layout_dst = GPUTexture::Layout::DepthStencilAttachment;
      }

      attachments.push_back(VkAttachmentDescription{
        .flags = 0,
        .format = (VkFormat)depth_stencil_attachment->format(),
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = (VkAttachmentLoadOp)depth_stencil_descriptor.load_op,
        .storeOp = (VkAttachmentStoreOp)depth_stencil_descriptor.store_op,
        .stencilLoadOp = (VkAttachmentLoadOp)depth_stencil_descriptor.stencil_load_op,
        .stencilStoreOp = (VkAttachmentStoreOp)depth_stencil_descriptor.stencil_store_op,
        .initialLayout = (VkImageLayout)layout_src,
        .finalLayout = (VkImageLayout)layout_dst
      });

      clear_values_.push_back(VkClearValue{
        .depthStencil = VkClearDepthStencilValue{
          .depth = 1,
          .stencil = 0
        }
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
      has_depth_stencil_attachment = true;
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

  auto GetNumberOfColorAttachments() -> size_t override {
    return color_attachment_count_;
  }

  bool HasDepthStencilAttachment() override {
    return has_depth_stencil_attachment;
  }

  auto GetClearValues() -> std::vector<VkClearValue> const& { return clear_values_; }

  void SetClearColor(int index, float r, float g, float b, float a) override {
    Assert(index < color_attachment_count_,
      "VulkanRenderPass: SetClearColor() called with an out-of-bounds index");

    // TODO: handle texture formats which don't use float32
    clear_values_[index].color = VkClearColorValue{
      .float32 = {r, g, b, a}
    };
  }

  void SetClearDepth(float depth) override {
    clear_values_.back().depthStencil.depth = depth;
  }

  void SetClearStencil(u32 stencil) override {
    clear_values_.back().depthStencil.stencil = stencil;
  }

private:
  VkDevice device_;
  VkRenderPass render_pass_;
  std::vector<VkClearValue> clear_values_;
  size_t color_attachment_count_;
  bool has_depth_stencil_attachment = false;
};

} // namespace Aura
