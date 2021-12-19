/*
 * Copyright (C) 2021 fleroviux
 */

#pragma once

#include <aurora/scene/game_object.hpp>
#include <aurora/integer.hpp>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

// TODO: move this somewhere more appropriate.
#include <aurora/scene/geometry/geometry.hpp>

namespace Aura {

// TODO: move this somewhere more appropriate.
struct MeshComponent final : Component {
  MeshComponent(GameObject* owner, Geometry* geometry)
      : Component(owner)
      , geometry(geometry) {
  }

  Geometry* geometry;
};

struct GLTFLoader {
  auto parse(std::string const& path) -> GameObject*;

private:
  using Buffer = std::vector<u8>;

  void load_buffers(nlohmann::json const& gltf);

  std::vector<Buffer> buffers_;
};

} // namespace Aura
