/*
 * Copyright (C) 2022 fleroviux
 */

#pragma once

#include <aurora/renderer/component/camera.hpp>
#include <aurora/scene/game_object.hpp>
#include <aurora/gal/texture.hpp>

namespace Aura {

struct RenderPipelineBase {
  virtual ~RenderPipelineBase() = default;

  virtual void Render(GameObject* scene, Camera* camera) = 0;

  virtual auto GetOutputTexture() -> GPUTexture* = 0;
};

} // namespace Aura
