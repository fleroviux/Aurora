/*
 * Copyright (C) 2021 fleroviux
 */

#pragma once

#include <aurora/renderer/texture.hpp>
#include <aurora/renderer/uniform_block.hpp>
#include <aurora/array_view.hpp>
#include <aurora/integer.hpp>
#include <aurora/log.hpp>
#include <string>
#include <unordered_map>
#include <vector>

#include "../../../src/pbr.glsl.hpp"

namespace Aura {

struct Material {
  Material(std::vector<std::string> const& compile_options = {}) {
    // TODO: validate that there aren't more than 24 compile options.
    for (size_t i = 0; i < compile_options.size(); i++) {
      compile_options_map_[compile_options[i]] = i;
    }
  }

  virtual ~Material() = default;

  virtual auto get_vert_shader() -> char const* = 0;
  virtual auto get_frag_shader() -> char const* = 0;
  virtual auto get_uniforms() -> UniformBlock& = 0;
  virtual auto get_texture_slots() -> ArrayView<std::shared_ptr<Texture>> = 0;

  auto get_compile_options() const -> u32 {
    return compile_options_;
  }

  void set_compile_option(std::string const& name, bool enable) {
    auto match = compile_options_map_.find(name);

    Assert(match != compile_options_map_.end(),
      "Material: unregistered compile option: {}", name);

    auto id = match->second;
    if (enable) {
      compile_options_ |=   1 << id;
    } else {
      compile_options_ &= ~(1 << id);
    }
  }

private:
  u32 compile_options_ = 0;
  std::unordered_map<std::string, size_t> compile_options_map_;
};

struct PbrMaterial final : Material {
  PbrMaterial() : Material({
    "ENABLE_ALBEDO_MAP",
    "ENABLE_METALNESS_MAP",
    "ENABLE_ROUGHNESS_MAP",
    "ENABLE_CLEARCOAT"
  }) {
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

  auto get_vert_shader() -> char const* override {
    return pbr_vert;
  }

  auto get_frag_shader() -> char const* override {
    return pbr_frag;
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
