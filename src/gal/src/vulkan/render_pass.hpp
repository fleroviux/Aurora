
// Copyright (C) 2022 fleroviux. All rights reserved.

#pragma once

#include <algorithm>
#include <array>
#include <aurora/gal/backend/vulkan.hpp>
#include <aurora/log.hpp>
#include <vector>

namespace Aura {

struct VulkanRenderPass final : RenderPass {
  VulkanRenderPass(
    VkDevice device,
    VkRenderPass render_pass,
    size_t color_attachment_count,
    bool have_depth_stencil_attachment
  )   : device_(device)
      , render_pass_(render_pass)
      , color_attachment_count_(color_attachment_count)
      , has_depth_stencil_attachment(have_depth_stencil_attachment) {
    clear_values_.resize(color_attachment_count_ + (have_depth_stencil_attachment ? 1 : 0));
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

struct VulkanRenderPassBuilder final : RenderPassBuilder {
  VulkanRenderPassBuilder(VkDevice device) : device(device) {
    const auto attachment_init = VkAttachmentDescription{
      .flags = 0,
      .format = VK_FORMAT_B8G8R8A8_SRGB,
      .samples = VK_SAMPLE_COUNT_1_BIT,
      .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
      .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
      .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
      .stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    for (size_t i = 0; i < kMaxColorAttachments; i++) {
      color_attachments[i] = attachment_init;
    }

    depth_stencil_attachment = attachment_init;
    depth_stencil_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
  }

  // TODO: auto-expand number of color attachments?

  void SetColorAttachmentCount(size_t count) override {
    color_attachment_count = count;
  }

  void SetColorAttachment(size_t id, AttachmentConfig const& config) override {
    SetColorAttachmentFormat(id, config.format);
    SetColorAttachmentSrcLayout(id, config.layout_src);
    SetColorAttachmentDstLayout(id, config.layout_dst);
    SetColorAttachmentLoadOp(id, config.load_op);
    SetColorAttachmentStoreOp(id, config.store_op);
  }

  void SetColorAttachmentFormat(size_t id, Texture::Format format) override {
    Expand(id);
    color_attachments.at(id).format = (VkFormat)format;
  }

  void SetColorAttachmentSrcLayout(size_t id, Texture::Layout layout) override {
    Expand(id);
    color_attachments.at(id).initialLayout = (VkImageLayout)layout;
  }

  void SetColorAttachmentDstLayout(size_t id, Texture::Layout layout) override {
    Expand(id);
    color_attachments.at(id).finalLayout = (VkImageLayout)layout;
  }

  void SetColorAttachmentLoadOp(size_t id, RenderPass::LoadOp op) override {
    Expand(id);
    color_attachments.at(id).loadOp = (VkAttachmentLoadOp)op;
  }

  void SetColorAttachmentStoreOp(size_t id, RenderPass::StoreOp op) override {
    Expand(id);
    color_attachments.at(id).storeOp = (VkAttachmentStoreOp)op;
  }

  void ClearDepthAttachment() override {
    have_depth_stencil_attachment = false;
  }

  void SetDepthAttachment(AttachmentConfig const& config) override {
    SetDepthAttachmentFormat(config.format);
    SetDepthAttachmentSrcLayout(config.layout_src);
    SetDepthAttachmentDstLayout(config.layout_dst);
    SetDepthAttachmentLoadOp(config.load_op);
    SetDepthAttachmentStoreOp(config.store_op);
    SetDepthAttachmentStencilLoadOp(config.load_op_stencil);
    SetDepthAttachmentStencilStoreOp(config.store_op_stencil);
  }

  void SetDepthAttachmentFormat(Texture::Format format) override {
    depth_stencil_attachment.format = (VkFormat)format;
    have_depth_stencil_attachment = true;
  }

  void SetDepthAttachmentSrcLayout(Texture::Layout layout) override {
    depth_stencil_attachment.initialLayout = (VkImageLayout)layout;
    have_depth_stencil_attachment = true;
  }

  void SetDepthAttachmentDstLayout(Texture::Layout layout) override {
    depth_stencil_attachment.finalLayout = (VkImageLayout)layout;
    have_depth_stencil_attachment = true;
  }

  void SetDepthAttachmentLoadOp(RenderPass::LoadOp op) override {
    depth_stencil_attachment.loadOp = (VkAttachmentLoadOp)op;
    have_depth_stencil_attachment = true;
  }

  void SetDepthAttachmentStoreOp(RenderPass::StoreOp op) override {
    depth_stencil_attachment.storeOp = (VkAttachmentStoreOp)op;
    have_depth_stencil_attachment = true;
  }

  void SetDepthAttachmentStencilLoadOp(RenderPass::LoadOp op) override {
    depth_stencil_attachment.stencilLoadOp = (VkAttachmentLoadOp)op;
    have_depth_stencil_attachment = true;
  }

  void SetDepthAttachmentStencilStoreOp(RenderPass::StoreOp op) override {
    depth_stencil_attachment.stencilStoreOp = (VkAttachmentStoreOp)op;
    have_depth_stencil_attachment = true;
  }

  auto Build() const -> std::unique_ptr<RenderPass> override {
    bool have_color_layout_transition = false;
    bool have_depth_layout_transition = false;

    size_t attachment_count = color_attachment_count;

    if (have_depth_stencil_attachment) {
      attachment_count++;
    }

    VkAttachmentDescription attachments[attachment_count];
    VkAttachmentReference references[attachment_count];

    for (size_t i = 0; i < color_attachment_count; i++) {
      attachments[i] = color_attachments[i];
      references[i].attachment = i;
      references[i].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

      auto final_layout = attachments[i].finalLayout;

      if (final_layout != VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL &&
          final_layout != VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
      ) {
        have_color_layout_transition = true;
      }
    }

    if (have_depth_stencil_attachment) {
      auto& attachment = attachments[color_attachment_count];
      auto& reference = references[color_attachment_count];

      attachment = depth_stencil_attachment;
      reference.attachment = color_attachment_count;
      reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

      if (attachment.finalLayout != VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        have_depth_layout_transition = true;
      }
    }

    auto subpass = VkSubpassDescription{};
    subpass.flags = 0;
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.inputAttachmentCount = 0;
    subpass.pInputAttachments = nullptr;
    subpass.colorAttachmentCount = color_attachment_count;
    subpass.pColorAttachments = references;
    subpass.pResolveAttachments = nullptr;

    if (have_depth_stencil_attachment) {
      subpass.pDepthStencilAttachment = &references[color_attachment_count];
    } else {
      subpass.pDepthStencilAttachment = nullptr;
    }

    auto info = VkRenderPassCreateInfo{};
    info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    info.pNext = nullptr;
    info.flags = 0;
    info.attachmentCount = attachment_count;
    info.pAttachments = attachments;
    info.subpassCount = 1;
    info.pSubpasses = &subpass;

    VkSubpassDependency dependency;

    if (have_color_layout_transition || have_depth_layout_transition) {
      // TODO: expose dstStageMask and dstAccessMask to the user.
      dependency.srcSubpass = 0;
      dependency.dstSubpass = VK_SUBPASS_EXTERNAL;
      dependency.srcStageMask = 0;
      dependency.dstStageMask = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
      dependency.srcAccessMask = 0;
      dependency.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
      dependency.dependencyFlags = 0;

      if (have_color_layout_transition) {
        dependency.srcStageMask |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask |= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
      }

      if (have_depth_layout_transition) {
        // TODO: confirm that the srcStageMask is correct.
        dependency.srcStageMask |= VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        dependency.srcAccessMask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
      }

      info.dependencyCount = 1;
      info.pDependencies = &dependency;
    } else {
      info.dependencyCount = 0;
      info.pDependencies = nullptr;
    }

    VkRenderPass render_pass;

    if (vkCreateRenderPass(device, &info, nullptr, &render_pass) != VK_SUCCESS) {
      Assert(false, "VulkanRenderPassBuilder: failed to create a render pass");
    }

    return std::make_unique<VulkanRenderPass>(device, render_pass, color_attachment_count, have_depth_stencil_attachment);
  }

private:
  // TODO: keep this constant somewhere globally?
  static constexpr size_t kMaxColorAttachments = 32;

  void Expand(size_t id) {
    color_attachment_count = std::max(color_attachment_count, id + 1);
  }

  VkDevice device;

  size_t color_attachment_count = 0;
  std::array<VkAttachmentDescription, kMaxColorAttachments> color_attachments;

  bool have_depth_stencil_attachment = false;
  VkAttachmentDescription depth_stencil_attachment;
};

} // namespace Aura
