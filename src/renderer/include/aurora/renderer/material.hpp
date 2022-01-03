/*
 * Copyright (C) 2021 fleroviux
 */

#pragma once

#include <aurora/renderer/texture.hpp>
#include <aurora/renderer/uniform_block_layout.hpp>
#include <aurora/array_view.hpp>

namespace Aura {

struct MaterialBase {
  virtual auto get_uniforms() const -> UniformBlock const& = 0;
  virtual auto get_texture_slots() -> ArrayView<std::shared_ptr<Texture>> = 0;
};

struct Material final : MaterialBase {
  Material() {
    auto layout = UniformBlockLayout{};
    layout.add<float>("metalness");
    layout.add<float>("roughness");
    uniforms_ = UniformBlock{layout};
  }

  auto metalness() -> float& {
    return uniforms_.get<float>("metalness");
  }

  auto roughness() -> float& {
    return uniforms_.get<float>("roughness");
  }

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

  auto get_uniforms() const -> UniformBlock const& override {
    return uniforms_;
  }

  auto get_texture_slots() -> ArrayView<std::shared_ptr<Texture>> override {
    return ArrayView{texture_slots_, 4};
  }

private:
  UniformBlock uniforms_;
  std::shared_ptr<Texture> texture_slots_[4];
};

} // namespace Aura
