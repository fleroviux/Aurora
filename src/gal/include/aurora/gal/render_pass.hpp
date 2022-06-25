
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

} // namespace Aura
