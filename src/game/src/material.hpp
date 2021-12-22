/*
 * Copyright (C) 2021 fleroviux
 */

#pragma once

#include "texture.hpp"

namespace Aura {

struct Material {
  std::shared_ptr<Texture> albedo;
};

} // namespace Aura
