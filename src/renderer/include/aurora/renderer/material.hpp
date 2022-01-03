/*
 * Copyright (C) 2021 fleroviux
 */

#pragma once

#include <aurora/renderer/texture.hpp>
#include <aurora/array_view.hpp>

namespace Aura {

struct MaterialBase {
  virtual auto get_texture_slots() -> ArrayView<std::shared_ptr<Texture>> = 0;
};

struct Material final : MaterialBase {
  auto albedo_map() -> std::shared_ptr<Texture>& {
    return texture_slots_[0];
  }

  auto metalness_map() -> std::shared_ptr<Texture>& {
    return texture_slots_[1];
  }

  auto roughness_map() -> std::shared_ptr<Texture>& {
    return texture_slots_[2];
  }

  auto normal_map() -> std::shared_ptr<Texture>& {
    return texture_slots_[3];
  }

  auto get_texture_slots() -> ArrayView<std::shared_ptr<Texture>> override {
    return ArrayView{texture_slots_, 4};
  }

  float roughness = 0.5;
  float metalness = 0.5;

private:
  std::shared_ptr<Texture> texture_slots_[4];
};

} // namespace Aura
