/*
 * Copyright (C) 2022 fleroviux
 */

#pragma once

namespace Aura {

struct RenderTarget {
  virtual ~RenderTarget() = default;

  virtual auto handle() -> void* = 0;
};

}; // namespace Aura
