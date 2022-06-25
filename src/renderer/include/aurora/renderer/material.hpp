
// Copyright (C) 2022 fleroviux. All rights reserved.

#pragma once

#include <aurora/gal/render_device.hpp>
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
  enum class Side {
    Front,
    Back,
    Both
  };

  struct BlendState {
    bool enable = false;
    BlendFactor src_color_factor = BlendFactor::SrcAlpha;
    BlendFactor dst_color_factor = BlendFactor::OneMinusSrcAlpha;
    BlendFactor src_alpha_factor = BlendFactor::SrcAlpha;
    BlendFactor dst_alpha_factor = BlendFactor::OneMinusSrcAlpha;
    BlendOp color_op = BlendOp::Add;
    BlendOp alpha_op = BlendOp::Add;
    float constants[4]{ 0, 0, 0, 0 };
  } blend_state;

  Material(std::vector<std::string> const& compile_options = {})
      : compile_option_names_(compile_options) {
    Assert(compile_options.size() <= 24,
      "Material: number of compile options is limited to 24");

    for (size_t i = 0; i < compile_options.size(); i++) {
      compile_options_map_[compile_options[i]] = i;
    }
  }

  virtual ~Material() = default;

  virtual auto get_vert_shader() -> char const* = 0;
  virtual auto get_frag_shader() -> char const* = 0;
  virtual auto get_uniforms() -> UniformBlock& = 0;
  virtual auto get_texture_slots() -> ArrayView<std::shared_ptr<Texture2D>> = 0;

  auto side() const -> Side {
    return side_;
  }

  auto side() -> Side& {
    return side_;
  }

  auto get_compile_options() const -> u32 {
    return compile_options_;
  }

  auto get_compile_option_names() const -> std::vector<std::string> const& {
    return compile_option_names_;
  }

protected:
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
  Side side_ = Side::Front;
  u32 compile_options_ = 0;
  std::unordered_map<std::string, size_t> compile_options_map_;
  std::vector<std::string> compile_option_names_;
};

struct PbrMaterial final : Material {
  PbrMaterial() : Material({
    "ENABLE_ALBEDO_MAP",
    "ENABLE_METALNESS_MAP",
    "ENABLE_ROUGHNESS_MAP",
    "ENABLE_NORMAL_MAP"
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

  auto get_albedo_map() -> std::shared_ptr<Texture2D> {
    return texture_slots_[0];
  }

  void set_albedo_map(std::shared_ptr<Texture2D> texture) {
    texture_slots_[0] = texture;
    set_compile_option("ENABLE_ALBEDO_MAP", (bool)texture);
  }

  auto get_metalness_map() -> std::shared_ptr<Texture2D> {
    return texture_slots_[1];
  }

  void set_metalness_map(std::shared_ptr<Texture2D> texture) {
    texture_slots_[1] = texture;
    set_compile_option("ENABLE_METALNESS_MAP", (bool)texture);
  }

  auto get_roughness_map() -> std::shared_ptr<Texture2D> {
    return texture_slots_[2];
  }

  void set_roughness_map(std::shared_ptr<Texture2D> texture) {
    texture_slots_[2] = texture;
    set_compile_option("ENABLE_ROUGHNESS_MAP", (bool)texture);
  }

  auto get_normal_map() -> std::shared_ptr<Texture2D> {
    return texture_slots_[3];
  }

  void set_normal_map(std::shared_ptr<Texture2D> texture) {
    texture_slots_[3] = texture;
    set_compile_option("ENABLE_NORMAL_MAP", (bool)texture);
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

  auto get_texture_slots() -> ArrayView<std::shared_ptr<Texture2D>> override {
    return ArrayView{texture_slots_, 4};
  }

private:
  UniformBlock uniforms_;
  std::shared_ptr<Texture2D> texture_slots_[4];
};

} // namespace Aura
