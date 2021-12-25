/*
 * Copyright (C) 2021 fleroviux
 */

#pragma once

#include <aurora/renderer/geometry/geometry.hpp>
#include <aurora/renderer/material.hpp>
#include <aurora/scene/component.hpp>
#include <memory>

namespace Aura {

struct MeshComponent final : Component {
  MeshComponent(
    GameObject* owner,
    std::shared_ptr<Geometry> geometry,
    std::shared_ptr<Material> material
  )   : Component(owner)
      , geometry(geometry)
      , material(material) {
  }

  std::shared_ptr<Geometry> geometry;
  std::shared_ptr<Material> material;
};

} // namespace Aura
