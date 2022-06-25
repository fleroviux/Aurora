
// Copyright (C) 2022 fleroviux. All rights reserved.

#pragma once

#include <aurora/gal/render_device.hpp>
#include <aurora/scene/game_object.hpp>
#include <memory>

namespace Aura {

struct RenderEngineOptions {
  std::shared_ptr<RenderDevice> render_device;
};

struct RenderEngineBase {
  virtual ~RenderEngineBase() = default;

  virtual void Render(
    GameObject* scene,
    std::array<std::unique_ptr<CommandBuffer>, 2>& command_buffers
  ) = 0;

  virtual auto GetOutputTexture() -> Texture* = 0;
};

auto CreateRenderEngine(
  RenderEngineOptions const& options
) -> std::unique_ptr<RenderEngineBase>;

} // namespace Aura
