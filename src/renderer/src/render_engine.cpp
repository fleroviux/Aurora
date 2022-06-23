/*
 * Copyright (C) 2022 fleroviux
 */

#include <aurora/renderer/render_engine.hpp>
#include <aurora/renderer/render_pipeline_base.hpp>

#include "forward_render_pipeline.hpp"

namespace Aura {

struct RenderEngine final : RenderEngineBase {
  RenderEngine() {
    CreateRenderPipeline();
  }

private:
  void CreateRenderPipeline() {
    render_pipeline = std::make_unique<ForwardRenderPipeline>();
  }

  std::unique_ptr<RenderPipelineBase> render_pipeline;
};

auto CreateRenderEngine() -> std::unique_ptr<RenderEngineBase> {
  return std::make_unique<RenderEngine>();
}

} // namespace Aura
