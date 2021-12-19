/*
 * Copyright (C) 2021 fleroviux
 */

#pragma once

#include <cstdint>

namespace Aura {

struct Updatable {
  bool needs_update() const {
    return needs_update_;
  }

  auto needs_update() -> bool& {
    return needs_update_;
  }

private:
  bool needs_update_ = true;
};

} // namespace Aura
