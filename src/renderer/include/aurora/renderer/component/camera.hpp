/*
 * Copyright (C) 2021 fleroviux
 */

#pragma once

#include <aurora/scene/component.hpp>

namespace Aura {

struct Camera final : Component {
  using Component::Component;

  Camera(
    GameObject* owner,
    double field_of_view,
    double aspect_ratio,
    double near,
    double far 
  )   : Component(owner)
      , field_of_view(field_of_view)
      , aspect_ratio(aspect_ratio)
      , near(near)
      , far(far) {
  }

  // Vertical field of view
  double field_of_view = 45.0;

  // Ratio of width to height (width divided by height)
  double aspect_ratio = 16 / 9.0;

  // Near camera clipping plane
  double near = 0.01;

  // Far camera clipping plane
  double far = 500.0;
};

} // namespace Aura
