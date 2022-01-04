/*
 * Copyright (C) 2021 fleroviux
 */

#pragma once

#include <aurora/renderer/texture.hpp>
#include <aurora/renderer/uniform_block.hpp>
#include <aurora/array_view.hpp>

namespace Aura {

struct Material {
  virtual ~Material() = default;

  virtual auto get_uniforms() -> UniformBlock& = 0;
  virtual auto get_texture_slots() -> ArrayView<std::shared_ptr<Texture>> = 0;
};

struct PbrMaterial final : Material {
  PbrMaterial() {
    auto layout = UniformBlockLayout{};
    layout.add<Matrix4>("model");
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

  auto get_uniforms() -> UniformBlock& override {
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
