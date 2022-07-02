
// Copyright (C) 2022 fleroviux. All rights reserved.

#pragma once

#include <array>
#include <aurora/gal/command_buffer.hpp>
#include <aurora/scene/game_object.hpp>
#include <aurora/gal/texture.hpp>
#include <memory>

namespace Aura {

struct RenderPipelineBase {
  virtual ~RenderPipelineBase() = default;

  virtual void Render(
    GameObject* scene,
    GameObject* camera,
    std::array<std::unique_ptr<CommandBuffer>, 2>& command_buffers
  ) = 0;

  virtual auto GetColorTexture() -> Texture* = 0;
  virtual auto GetDepthTexture() -> Texture* = 0;
  virtual auto GetNormalTexture() -> Texture* = 0;
};

} // namespace Aura
