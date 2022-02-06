/*
 * Copyright (C) 2022 fleroviux
 */

#pragma once

#include <aurora/gal/render_pass.hpp>
#include <vector>

namespace Aura {

struct RenderTarget {
  virtual ~RenderTarget() = default;

  virtual auto handle() -> void* = 0;
  
  virtual auto CreateRenderPass(
    std::vector<RenderPass::Descriptor> const& color_descriptors = {{}},
    RenderPass::Descriptor depth_stencil_descriptor = {}
  ) -> std::unique_ptr<RenderPass> = 0;
};

}; // namespace Aura
