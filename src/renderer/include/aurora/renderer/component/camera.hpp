/*
 * Copyright (C) 2021 fleroviux
 */

#pragma once

#include <aurora/scene/component.hpp>

namespace Aura {

struct PerspectiveCamera final : Component {
  using Component::Component;

  PerspectiveCamera(
    GameObject* owner,
    float field_of_view,
    float aspect_ratio,
    float near,
    float far 
  )   : Component(owner)
      , field_of_view(field_of_view)
      , aspect_ratio(aspect_ratio)
      , near(near)
      , far(far) {
  }

  float field_of_view = 45.0;
  float aspect_ratio = 16 / 9.0;
  float near = 0.01;
  float far = 500.0;
};

struct OrthographicCamera final : Component {
  using Component::Component;

  OrthographicCamera(
    GameObject* owner,
    float left,
    float right,
    float bottom,
    float top,
    float near,
    float far
  )   : Component(owner)
      , left(left)
      , right(right)
      , bottom(bottom)
      , top(top)
      , near(near)
      , far(far) {
  }

  float left = -1.0;
  float right = 1.0;
  float bottom = -1.0;
  float top = 1.0;
  float near = 0.01;
  float far = 500.0;
};

} // namespace Aura
