
// Copyright (C) 2022 fleroviux. All rights reserved.

#include <aurora/renderer/texture.hpp>
#include <stb_image.h>

namespace Aura {

auto Texture2D::load(std::string const& path) -> std::unique_ptr<Texture2D> {
  int width;
  int height;
  int components;
  auto data = stbi_load(path.c_str(), &width, &height, &components, STBI_rgb_alpha);

  Assert(data != nullptr, "Failed to load texture: {}", path);

  return std::make_unique<Texture2D>(width, height, data);
}

} // namespace Aura
