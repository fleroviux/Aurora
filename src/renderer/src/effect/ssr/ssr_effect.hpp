
// Copyright (C) 2022 fleroviux. All rights reserved.

#pragma once

#include <aurora/gal/render_device.hpp>
#include <aurora/gal/texture.hpp>
#include <aurora/renderer/uniform_block.hpp>
#include <aurora/scene/game_object.hpp>
#include <aurora/any_ptr.hpp>

namespace Aura {

struct SSREffect {
  SSREffect(std::shared_ptr<RenderDevice> render_device);

  void Render(
    GameObject* camera,
    AnyPtr<CommandBuffer> command_buffer,
    AnyPtr<GPUTexture> render_texture,
    AnyPtr<RenderTarget> render_target,
    AnyPtr<GPUTexture> color_texture,
    AnyPtr<GPUTexture> depth_texture,
    AnyPtr<GPUTexture> normal_texture
  );

private:
  void CreateUBO();
  void CreateBindGroupAndPipelineLayout();
  void CreateSampler();
  void CreateShaderModules();
  void CreateGraphicsPipeline(std::shared_ptr<RenderPass> render_pass);

  std::shared_ptr<RenderDevice> render_device;

  std::shared_ptr<BindGroupLayout> bind_group_layout;
  std::unique_ptr<BindGroup> bind_group;
  std::unique_ptr<Sampler> sampler;
  std::shared_ptr<PipelineLayout> pipeline_layout;
  std::shared_ptr<ShaderModule> shader_vert;
  std::shared_ptr<ShaderModule> shader_frag;
  std::shared_ptr<RenderPass> render_pass;
  std::unique_ptr<GraphicsPipeline> pipeline;

  // Uniforms
  UniformBlock uniform_block;
  std::unique_ptr<Buffer> uniform_buffer;

};

} // namespace Aura
