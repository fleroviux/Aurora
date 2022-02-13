/*
 * Copyright (C) 2022 fleroviux
 */

#pragma once

#include <aurora/gal/render_target.hpp>
#include <aurora/gal/render_pass.hpp>
#include <aurora/any_ptr.hpp>

namespace Aura {

struct CommandBuffer {
  enum class OneTimeSubmit {
    No = 0,
    Yes = 1
  };

  virtual ~CommandBuffer() = default;

  virtual auto Handle() -> void* = 0;
  virtual void Begin(OneTimeSubmit one_time_submit) = 0;
  virtual void End() = 0;
  virtual void BeginRenderPass(
    AnyPtr<RenderTarget> render_target,
    AnyPtr<RenderPass> render_pass
  ) = 0;
  virtual void EndRenderPass() = 0;
};

} // namespace Aura
