/*
 * Copyright (C) 2021 fleroviux
 */

#pragma once

#include <aurora/renderer/texture.hpp>

namespace Aura {

struct Material {
  float roughness = 0.5;
  float metalness = 0.5;

  // TODO: rename albedo to albedo_map
  std::shared_ptr<Texture> albedo;
  std::shared_ptr<Texture> metalness_map;
  std::shared_ptr<Texture> roughness_map;
};

} // namespace Aura
