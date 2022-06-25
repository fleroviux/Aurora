
// Copyright (C) 2022 fleroviux. All rights reserved.

#pragma once

#include <aurora/renderer/geometry/geometry.hpp>
#include <aurora/renderer/material.hpp>
#include <aurora/scene/component.hpp>
#include <memory>

namespace Aura {

struct Mesh final : Component {
  Mesh(
    GameObject* owner,
    std::shared_ptr<Geometry> geometry,
    std::shared_ptr<Material> material
  )   : Component(owner)
      , geometry(geometry)
      , material(material) {
  }

  std::shared_ptr<Geometry> geometry;
  std::shared_ptr<Material> material;

  bool visible = true;
};

} // namespace Aura
