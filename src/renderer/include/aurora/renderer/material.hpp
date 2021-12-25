/*
 * Copyright (C) 2021 fleroviux
 */

#pragma once

#include <aurora/renderer/texture.hpp>

namespace Aura {

struct Material {
  std::shared_ptr<Texture> albedo;
};

} // namespace Aura
