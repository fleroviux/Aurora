/*
 * Copyright (C) 2022 fleroviux
 */

#pragma once

#include <functional>
#include <vector>

namespace Aura {

struct GPUResource {
  virtual ~GPUResource() {
    for (auto& callback : release_callbacks_) callback();
  }

  bool needs_update() const {
    return needs_update_;
  }

  bool& needs_update() {
    return needs_update_;
  }

  void add_release_callback(std::function<void()> callback) const {
    const_cast<GPUResource*>(this)->release_callbacks_.push_back(callback);
  }

private:
  bool needs_update_ = true;
  std::vector<std::function<void()>> release_callbacks_;
};

} // namespace Aura
