/*
 * Copyright (C) 2022 fleroviux
 */

#pragma once

#include <aurora/renderer/render_pipeline_base.hpp>

namespace Aura {

struct ForwardRenderPipeline final : RenderPipelineBase {
  void Render(GameObject* scene, Camera* camera) override;

  auto GetOutputTexture() -> GPUTexture* override;
};

} // namespace Aura
