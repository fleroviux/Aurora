/*
 * Copyright (C) 2021 fleroviux
 */

#pragma once

#include <aurora/integer.hpp>
#include <aurora/log.hpp>
#include <memory>
#include <stb_image.h>
#include <string>

namespace Aura {

struct Texture{
  Texture(uint width, uint height, u8* data)
      : width_(width)
      , height_(height)
      , data_(data) {
  }

 ~Texture() {
    delete data();
  }

  static auto load(std::string const& path) -> std::unique_ptr<Texture> {
    int width;
    int height;
    int components;
    auto data = stbi_load(path.c_str(), &width, &height, &components, STBI_rgb_alpha);

    Assert(data != nullptr, "Failed to load texture: {}", path);

    return std::make_unique<Texture>(width, height, data);
  }

  auto data() const -> u8 const* {
    return data_;
  }

  auto width() const -> uint {
    return width_;
  }

  auto height() const -> uint {
    return height_;
  }

private:
  uint width_;
  uint height_;
  u8* data_;
};

} // namespace Aura
