/*
 * Copyright (C) 2022 fleroviux
 */

#include <aurora/renderer/component/camera.hpp>
#include <aurora/log.hpp>
#include <shaderc/shaderc.hpp>

#include "shader/raytrace.glsl.hpp"
#include "ssr_effect.hpp"

namespace Aura {

SSREffect::SSREffect(std::shared_ptr<RenderDevice> render_device)
    : render_device(render_device) {
  CreateUBO();
  CreateBindGroupAndPipelineLayout();
  CreateSampler();
  CreateShaderModules();
  //CreateGraphicsPipeline();
}

void SSREffect::Render(
  GameObject* camera,
  AnyPtr<CommandBuffer> command_buffer,
  AnyPtr<GPUTexture> render_texture,
  AnyPtr<RenderTarget> render_target,
  AnyPtr<GPUTexture> color_texture,
  AnyPtr<GPUTexture> depth_texture,
  AnyPtr<GPUTexture> normal_texture
) {
  // TODO(fleroviux): fix this absolutely atrocious terribleness
  if (pipeline == nullptr) {
    render_pass = render_target->CreateRenderPass({ {
    } });
    CreateGraphicsPipeline(render_pass);
  }

  if (camera->has_component<PerspectiveCamera>()) {
    // TODO: cache pointers to the uniform block members
    auto cam = camera->get_component<PerspectiveCamera>();
    auto& projection = cam->get_projection();
    uniform_block.get<Matrix4>("projection") = projection;
    uniform_block.get<Matrix4>("projection_inverse") = projection.inverse();
  } else {
    Assert(false, "SSREffect: unsupported camera type");
  }

  uniform_buffer->Update(uniform_block.data(), uniform_block.size());

  bind_group->Bind(0, color_texture, sampler, GPUTexture::Layout::ShaderReadOnly);
  bind_group->Bind(1, depth_texture, sampler, GPUTexture::Layout::DepthReadOnly);
  bind_group->Bind(2, normal_texture, sampler, GPUTexture::Layout::ShaderReadOnly);

  command_buffer->BeginRenderPass(render_target, render_pass);
  command_buffer->BindGraphicsPipeline(pipeline);
  command_buffer->BindGraphicsBindGroup(0, pipeline_layout, bind_group);
  command_buffer->Draw(3);
  command_buffer->EndRenderPass();

  auto barrier = MemoryBarrier{
    render_texture,
    Access::ColorAttachmentWrite,
    Access::ShaderRead,
    GPUTexture::Layout::ColorAttachment,
    GPUTexture::Layout::ShaderReadOnly
  };

  command_buffer->PipelineBarrier(
    PipelineStage::ColorAttachmentOutput,
    PipelineStage::FragmentShader,
    {&barrier, 1}
  );
}

void SSREffect::CreateUBO() {
  auto layout = UniformBlockLayout{};
  layout.add<Matrix4>("projection");
  layout.add<Matrix4>("projection_inverse");

  uniform_block = UniformBlock{ layout };
  uniform_buffer = render_device->CreateBuffer(Buffer::Usage::UniformBuffer, uniform_block.size());
}

void SSREffect::CreateBindGroupAndPipelineLayout() {
  bind_group_layout = render_device->CreateBindGroupLayout({ {
    .binding = 0,
    .type = BindGroupLayout::Entry::Type::ImageWithSampler
  }, {
    .binding = 1,
    .type = BindGroupLayout::Entry::Type::ImageWithSampler
  }, {
    .binding = 2,
    .type = BindGroupLayout::Entry::Type::ImageWithSampler
  }, {
    .binding = 3,
    .type = BindGroupLayout::Entry::Type::UniformBuffer
  }});
  bind_group = bind_group_layout->Instantiate();
  bind_group->Bind(3, uniform_buffer, BindGroupLayout::Entry::Type::UniformBuffer);

  pipeline_layout = render_device->CreatePipelineLayout({bind_group_layout});
}

void SSREffect::CreateSampler() {
  // TODO: have the render device expose default samplers.
  sampler = render_device->CreateSampler(Sampler::Config{});
}

void SSREffect::CreateShaderModules() {
  // TODO: move compiler code outside of this class.

  auto compiler = shaderc::Compiler{};
  auto options = shaderc::CompileOptions{};

  options.SetOptimizationLevel(shaderc_optimization_level_performance);

  auto result_vert = compiler.CompileGlslToSpv(
    raytrace_vert,
    shaderc_shader_kind::shaderc_vertex_shader,
    "main.vert",
    options
  );

  auto status_vert = result_vert.GetCompilationStatus();
  if (status_vert != shaderc_compilation_status_success) {
    Log<Error>("SSREffect: failed to compile vertex shader ({}):\n{}", status_vert, result_vert.GetErrorMessage());
  }

  auto result_frag = compiler.CompileGlslToSpv(
    raytrace_frag,
    shaderc_shader_kind::shaderc_fragment_shader,
    "main.frag",
    options
  );

  auto status_frag = result_frag.GetCompilationStatus();
  if (status_frag != shaderc_compilation_status_success) {
    Log<Error>("SSREffect: failed to compile fragment shader ({}):\n{}", status_frag, result_frag.GetErrorMessage());
  }

  auto spirv_vert = std::vector<u32>{result_vert.cbegin(), result_vert.cend()};
  auto spirv_frag = std::vector<u32>{result_frag.cbegin(), result_frag.cend()};

  shader_vert = render_device->CreateShaderModule(spirv_vert.data(), spirv_vert.size() * sizeof(u32));
  shader_frag = render_device->CreateShaderModule(spirv_frag.data(), spirv_frag.size() * sizeof(u32));
}

void SSREffect::CreateGraphicsPipeline(std::shared_ptr<RenderPass> render_pass) {
  auto builder = render_device->CreateGraphicsPipelineBuilder();

  builder->SetViewport(0, 0, 3200, 1800);
  builder->SetShaderModule(PipelineStage::VertexShader, shader_vert);
  builder->SetShaderModule(PipelineStage::FragmentShader, shader_frag);
  builder->SetPipelineLayout(pipeline_layout);
  builder->SetRenderPass(render_pass);

  pipeline = builder->Build();
}

} // namespace Aura
