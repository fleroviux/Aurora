/*
 * Copyright (C) 2021 fleroviux
 */

#include <aurora/renderer/texture.hpp>

namespace Aura {

auto Texture::load(std::string const& path) -> std::unique_ptr<Texture> {
  int width;
  int height;
  int components;
  auto data = stbi_load(path.c_str(), &width, &height, &components, STBI_rgb_alpha);

  Assert(data != nullptr, "Failed to load texture: {}", path);

  return std::make_unique<Texture>(width, height, data);
}

} // namespace Aura
