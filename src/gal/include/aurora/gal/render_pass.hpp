
// Copyright (C) 2022 fleroviux. All rights reserved.

#pragma once

#include <aurora/integer.hpp>
#include <aurora/gal/texture.hpp>

namespace Aura {

struct RenderPass {
  // https://vulkan.lunarg.com/doc/view/latest/windows/apispec.html#VkAttachmentLoadOp
  enum class LoadOp {
    Load = 0,
    Clear = 1,
    DontCare = 2
  };

  // https://vulkan.lunarg.com/doc/view/latest/windows/apispec.html#VkAttachmentStoreOp
  enum class StoreOp {
    Store = 0,
    DontCare = 1
  };

  struct Descriptor {
    LoadOp load_op = LoadOp::Clear;
    LoadOp stencil_load_op = LoadOp::DontCare;
    StoreOp store_op = StoreOp::Store;
    StoreOp stencil_store_op = StoreOp::DontCare;
    Texture::Layout layout_src = Texture::Layout::Undefined;
    Texture::Layout layout_dst = Texture::Layout::ColorAttachment;
  };

  virtual ~RenderPass() = default;

  virtual auto GetNumberOfColorAttachments() -> size_t = 0;
  virtual bool HasDepthStencilAttachment() = 0;

  virtual void SetClearColor(int index, float r, float g, float b, float a) = 0;
  virtual void SetClearDepth(float depth) = 0;
  virtual void SetClearStencil(u32 stencil) = 0;
};

struct RenderPassBuilder {
  struct AttachmentConfig {
    Texture::Format format;
    Texture::Layout layout_src;
    Texture::Layout layout_dst;

    RenderPass::LoadOp load_op = RenderPass::LoadOp::Clear;
    RenderPass::StoreOp store_op = RenderPass::StoreOp::Store;

    RenderPass::LoadOp load_op_stencil = RenderPass::LoadOp::DontCare;
    RenderPass::StoreOp store_op_stencil = RenderPass::StoreOp::DontCare;
  };

  virtual ~RenderPassBuilder() = default;

  virtual void SetColorAttachmentCount(size_t count) = 0;
  virtual void SetColorAttachment(size_t id, AttachmentConfig const& config) = 0;
  virtual void SetColorAttachmentFormat(size_t id, Texture::Format format) = 0;
  virtual void SetColorAttachmentSrcLayout(size_t id, Texture::Layout layout) = 0;
  virtual void SetColorAttachmentDstLayout(size_t id, Texture::Layout layout) = 0;
  virtual void SetColorAttachmentLoadOp(size_t id, RenderPass::LoadOp op) = 0;
  virtual void SetColorAttachmentStoreOp(size_t id, RenderPass::StoreOp op) = 0;

  virtual void ClearDepthAttachment() = 0;
  virtual void SetDepthAttachment(AttachmentConfig const& config) = 0;
  virtual void SetDepthAttachmentFormat(Texture::Format format) = 0;
  virtual void SetDepthAttachmentSrcLayout(Texture::Layout layout) = 0;
  virtual void SetDepthAttachmentDstLayout(Texture::Layout layout) = 0;
  virtual void SetDepthAttachmentLoadOp(RenderPass::LoadOp op) = 0;
  virtual void SetDepthAttachmentStoreOp(RenderPass::StoreOp op) = 0;
  virtual void SetDepthAttachmentStencilLoadOp(RenderPass::LoadOp op) = 0;
  virtual void SetDepthAttachmentStencilStoreOp(RenderPass::StoreOp op) = 0;

  virtual auto Build() const -> std::unique_ptr<RenderPass> = 0;
};

} // namespace Aura
