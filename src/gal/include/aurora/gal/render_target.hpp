
// Copyright (C) 2022 fleroviux. All rights reserved.

#pragma once

#include <aurora/gal/render_pass.hpp>
#include <vector>

namespace Aura {

struct RenderTarget {
  virtual ~RenderTarget() = default;

  virtual auto handle() -> void* = 0;
  virtual auto width()  -> u32 = 0;
  virtual auto height() -> u32 = 0;
};

}; // namespace Aura
