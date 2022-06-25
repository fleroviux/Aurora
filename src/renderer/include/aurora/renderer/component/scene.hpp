
// Copyright (C) 2022 fleroviux. All rights reserved.

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
