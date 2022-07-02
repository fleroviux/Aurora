
// Copyright (C) 2022 fleroviux. All rights reserved.

#pragma once

#include <aurora/utility.hpp>

namespace Aura {

#define AURA_ASSERT_IS_COMPONENT(T) \
  static_assert(std::is_base_of_v<Component, T>,\
    "AuroraScene: T must inherit from Aura::Component");

#define AURA_ASSERT_IS_REMOVABLE_COMPONENT(T) \
  AURA_ASSERT_IS_COMPONENT(T)\
  static_assert(T::removable(),\
    "AuroraScene: T must be a removable Aura::Component");

struct GameObject;

struct Component : NonCopyable, NonMovable {
  Component(GameObject* owner) : owner_(owner) {
  }

  auto owner() const -> GameObject const* {
    return owner_;
  }

  auto owner() -> GameObject* {
    return owner_;
  }

  static constexpr bool removable() {
    return true;
  }

  virtual ~Component() = default;

private:
  GameObject* owner_;
};

} // namespace Aura
