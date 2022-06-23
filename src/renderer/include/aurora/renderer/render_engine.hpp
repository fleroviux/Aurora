/*
 * Copyright (C) 2022 fleroviux
 */

#pragma once

#include <memory>

namespace Aura {

struct RenderEngineBase {
  virtual ~RenderEngineBase() = 0;
};

auto CreateRenderEngine() -> std::unique_ptr<RenderEngineBase>;

} // namespace Aura
