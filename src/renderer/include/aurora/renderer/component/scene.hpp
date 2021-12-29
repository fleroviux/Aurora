/*
 * Copyright (C) 2021 fleroviux
 */

#pragma once

#include <aurora/scene/component.hpp>

namespace Aura {

struct Scene final : Component {
  Scene(GameObject* owner, GameObject* camera)
      : Component(owner)
      , camera(camera) {
  }

  GameObject* camera;
};

} // namespace Aura
