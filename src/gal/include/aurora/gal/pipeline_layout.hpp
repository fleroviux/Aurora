
// Copyright (C) 2022 fleroviux. All rights reserved.

#pragma once

namespace Aura {

struct PipelineLayout {
  virtual ~PipelineLayout() = default;

  virtual auto Handle() -> void* = 0;
};

} // namespace Aura
