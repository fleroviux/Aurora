/*
 * Copyright (C) 2022 fleroviux
 */

#pragma once

#include <aurora/integer.hpp>

namespace Aura {

struct CommandPool {
  // subset of VkCommandPoolCreateFlagBits:
  // https://vulkan.lunarg.com/doc/view/latest/windows/apispec.html#VkCommandPoolCreateFlagBits
  enum class Usage : u32 {
    Transient = 1,
    ResetCommandBuffer = 2
  };

  virtual ~CommandPool() = default;

  virtual auto Handle() -> void* = 0;
};

constexpr auto operator|(
  CommandPool::Usage lhs,
  CommandPool::Usage rhs
) -> CommandPool::Usage {
  return (CommandPool::Usage)((u32)lhs | (u32)rhs);
}

} // namespace Aura
