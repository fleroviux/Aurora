
// Copyright (C) 2022 fleroviux. All rights reserved.

#pragma once

#include <aurora/renderer/gpu_resource.hpp>
#include <aurora/integer.hpp>
#include <aurora/log.hpp>
#include <aurora/utility.hpp>
#include <memory>
#include <string>

namespace Aura {

struct Texture2D final : GPUResource, NonCopyable, NonMovable {
  Texture2D(uint width, uint height, u8 * data)
      : width_(width)
      , height_(height)
      , data_(data) {
  }

 ~Texture2D() override {
    delete data();
  }

  auto data() const -> u8 const* {
    return data_;
  }

  auto data() -> u8* {
    return data_;
  }

  auto width() const -> uint {
    return width_;
  }

  auto height() const -> uint {
    return height_;
  }

  static auto load(std::string const& path) -> std::unique_ptr<Texture2D>;

private:
  uint width_;
  uint height_;
  u8* data_;
};

} // namespace Aura
