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

  auto data() const -> u8 const* {
    return data_;
  }

  auto width() const -> uint {
    return width_;
  }

  auto height() const -> uint {
    return height_;
  }

  static auto load(std::string const& path) -> std::unique_ptr<Texture>;

private:
  uint width_;
  uint height_;
  u8* data_;
};

} // namespace Aura
