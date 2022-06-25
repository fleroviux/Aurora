/*
 * Copyright (C) 2022 fleroviux
 */

#include <algorithm>
#include <shaderc/shaderc.hpp>
#include <vector>

#include "forward_render_pipeline.hpp"

namespace Aura {

ForwardRenderPipeline::ForwardRenderPipeline(
  std::shared_ptr<RenderDevice> render_device,
  std::shared_ptr<GeometryCache> geometry_cache
)   : render_device(render_device), geometry_cache(geometry_cache) {
  CreateCameraUniformBlock();
  CreateRenderTarget();
  CreateBindGroupAndPipelineLayout();
}

void ForwardRenderPipeline::Render(
  GameObject* scene,
  GameObject* camera,
  std::array<std::unique_ptr<CommandBuffer>, 2>& command_buffers
) {
  std::vector<Renderable> render_list_opaque;
  std::vector<Renderable> render_list_transparent;

  const std::function<void(GameObject*)> record_render_list = [&](GameObject* object) {
    if (!object->visible()) {
      return;
    }

    auto& transform = object->transform();
    if (transform.auto_update()) {
      transform.update_local();
    }
    transform.update_world(false);

    auto mesh = object->get_component<Mesh>();
    auto modelview = *camera_data.view * transform.world();

    // TODO: calculate view * world here and use it both for sorting and for culling.
    if (mesh && mesh->visible && IsObjectInsideCameraFrustum(modelview, mesh->geometry)) {
      auto& position = transform.position();
      auto  view_z = modelview.x().z() * position.x() +
                     modelview.y().z() * position.y() +
                     modelview.z().z() * position.z() +
                     modelview.w().z();

      if (mesh->material->blend_state.enable) {
        render_list_transparent.push_back({object, mesh, view_z});
      } else {
        render_list_opaque.push_back({object, mesh, view_z});
      }
    }

    for (auto child : object->children()) record_render_list(child);
  };

  if (!uploaded_example_cubemap) {
    CreateExampleCubeMap((VkCommandBuffer)command_buffers[0]->Handle());
    uploaded_example_cubemap = true;
  }
  
  UpdateCamera(camera);

  record_render_list(scene);

  const auto comparator_lt = [](Renderable& a, Renderable& b) {
    return a.z < b.z;
  };
  const auto comparator_gt = [](Renderable& a, Renderable& b) {
    return a.z > b.z;
  };
  std::sort(render_list_opaque.begin(), render_list_opaque.end(), comparator_lt);
  std::sort(render_list_transparent.begin(), render_list_transparent.end(), comparator_lt);

  command_buffers[1]->BeginRenderPass(render_target, render_pass);

  for (auto const& renderable : render_list_opaque) {
    RenderObject(command_buffers, renderable.object, renderable.mesh);
  }

  for (auto const& renderable : render_list_transparent) {
    RenderObject(command_buffers, renderable.object, renderable.mesh);
  }

  command_buffers[1]->EndRenderPass();

  // TODO: use a subpass dependency to transition render target image layouts.
  MemoryBarrier barriers[]{
    {
      color_texture,
      Access::ColorAttachmentWrite,
      Access::ShaderRead,
      GPUTexture::Layout::ColorAttachment,
      GPUTexture::Layout::ShaderReadOnly
    },
    {
      albedo_texture,
      Access::ColorAttachmentWrite,
      Access::ShaderRead,
      GPUTexture::Layout::ColorAttachment,
      GPUTexture::Layout::ShaderReadOnly
    },
    {
      normal_texture,
      Access::ColorAttachmentWrite,
      Access::ShaderRead,
      GPUTexture::Layout::ColorAttachment,
      GPUTexture::Layout::ShaderReadOnly
    },
    {
      depth_texture,
      Access::DepthStencilAttachmentWrite,
      Access::ShaderRead,
      GPUTexture::Layout::DepthStencilAttachment,
      GPUTexture::Layout::DepthReadOnly,
      {
        GPUTexture::Aspect::Depth
      }
    }
  };

  command_buffers[1]->PipelineBarrier(
    PipelineStage::ColorAttachmentOutput | PipelineStage::LateFragmentTests,
    PipelineStage::FragmentShader,
    {barriers, 4}
  );
}

auto ForwardRenderPipeline::GetColorTexture() -> GPUTexture* {
  return color_texture.get();
}

auto ForwardRenderPipeline::GetDepthTexture() -> GPUTexture* {
  return depth_texture.get();
}

auto ForwardRenderPipeline::GetNormalTexture() -> GPUTexture* {
  return normal_texture.get();
}

void ForwardRenderPipeline::CreateCameraUniformBlock() {
  auto layout = UniformBlockLayout{};
  layout.add<Matrix4>("projection");
  layout.add<Matrix4>("view");

  camera_data.data = UniformBlock{layout};
  camera_data.projection = &camera_data.data.get<Matrix4>("projection");
  camera_data.view = &camera_data.data.get<Matrix4>("view");
  camera_data.ubo = render_device->CreateBuffer(Buffer::Usage::UniformBuffer, camera_data.data.size());
}

void ForwardRenderPipeline::CreateRenderTarget() {
  color_texture = render_device->CreateTexture2D(
    3200,
    1800,
    GPUTexture::Format::B8G8R8A8_SRGB,
    GPUTexture::Usage::ColorAttachment | GPUTexture::Usage::Sampled
  );

  // TODO: use the proper format.
  albedo_texture = render_device->CreateTexture2D(
    3200,
    1800,
    GPUTexture::Format::B8G8R8A8_SRGB,
    GPUTexture::Usage::ColorAttachment | GPUTexture::Usage::Sampled
  );

  // TODO: use the proper format.
  normal_texture = render_device->CreateTexture2D(
    3200,
    1800,
    GPUTexture::Format::B8G8R8A8_SRGB,
    GPUTexture::Usage::ColorAttachment | GPUTexture::Usage::Sampled
  );

  depth_texture = render_device->CreateTexture2D(
    3200,
    1800,
    GPUTexture::Format::DEPTH_F32,
    GPUTexture::Usage::DepthStencilAttachment | GPUTexture::Usage::Sampled
  );

  render_target = render_device->CreateRenderTarget({color_texture, albedo_texture, normal_texture}, depth_texture);
  render_pass = render_target->CreateRenderPass();

  render_pass->SetClearColor(0, 0.01, 0.01, 0.01, 1.0);
  render_pass->SetClearColor(1, 0.00, 0.00, 0.00, 0.5);
  render_pass->SetClearColor(2, 0.50, 0.50, 0.50, 0.5);
  render_pass->SetClearDepth(1);
}

void ForwardRenderPipeline::CreateBindGroupAndPipelineLayout() {
  auto bindings = std::vector<BindGroupLayout::Entry>{
    {
      .binding = 0,
      .type = BindGroupLayout::Entry::Type::UniformBuffer
    },
    {
      .binding = 1,
      .type = BindGroupLayout::Entry::Type::UniformBuffer
    },
    {
      .binding = 2,
      .type = BindGroupLayout::Entry::Type::UniformBuffer
    }
  };

  for (int i = 0; i < 32; i++) {
    bindings.push_back({
      .binding = (u32)bindings.size(),
      .type = BindGroupLayout::Entry::Type::ImageWithSampler
      });
  }

  bind_group_layout = render_device->CreateBindGroupLayout(bindings);
  pipeline_layout = render_device->CreatePipelineLayout({bind_group_layout});
}

auto ForwardRenderPipeline::CreatePipeline(
  AnyPtr<Geometry> geometry,
  AnyPtr<Material> material,
  std::shared_ptr<PipelineLayout>& pipeline_layout,
  std::shared_ptr<ShaderModule>& shader_vert,
  std::shared_ptr<ShaderModule>& shader_frag
) -> std::unique_ptr<GraphicsPipeline> {
  auto pipeline_builder = render_device->CreateGraphicsPipelineBuilder();

  // TODO: negate y-component in the graphics backend?
  pipeline_builder->SetViewport(0, 1800, 3200, -1800);
  pipeline_builder->SetScissor(0, 0, 3200, 1800);
  pipeline_builder->SetShaderModule(PipelineStage::VertexShader, shader_vert);
  pipeline_builder->SetShaderModule(PipelineStage::FragmentShader, shader_frag);
  pipeline_builder->SetPipelineLayout(pipeline_layout);
  pipeline_builder->SetRenderPass(render_pass);
  pipeline_builder->SetRasterizerDiscardEnable(false);
  pipeline_builder->SetPolygonMode(PolygonMode::Fill);
  pipeline_builder->SetDepthTestEnable(true);
  pipeline_builder->SetDepthWriteEnable(true);
  pipeline_builder->SetDepthCompareOp(CompareOp::LessOrEqual);
  pipeline_builder->SetPrimitiveTopology(PrimitiveTopology::TriangleList);
  pipeline_builder->SetPrimitiveRestartEnable(false);

  switch (material->side()) {
    case Material::Side::Back:
      pipeline_builder->SetPolygonCull(PolygonFace::Front);
      break;
    case Material::Side::Front:
      pipeline_builder->SetPolygonCull(PolygonFace::Back);
      break;
    default:
      pipeline_builder->SetPolygonCull(PolygonFace::None);
      break;
  }

  auto& blend_state = material->blend_state;

  if (blend_state.enable) {
    pipeline_builder->SetBlendEnable(0, true);
    pipeline_builder->SetSrcColorBlendFactor(0, blend_state.src_color_factor);
    pipeline_builder->SetDstColorBlendFactor(0, blend_state.dst_color_factor);
    pipeline_builder->SetSrcAlphaBlendFactor(0, blend_state.src_alpha_factor);
    pipeline_builder->SetDstAlphaBlendFactor(0, blend_state.dst_alpha_factor);
    pipeline_builder->SetColorBlendOp(0, blend_state.color_op);
    pipeline_builder->SetAlphaBlendOp(0, blend_state.alpha_op);
    pipeline_builder->SetBlendConstants(blend_state.constants[0], blend_state.constants[1], blend_state.constants[2], blend_state.constants[3]);
  }

  u32 binding = 0;

  for (auto& buffer : geometry->get_vertex_buffers()) {
    pipeline_builder->AddVertexInputBinding(binding++, buffer->stride());
  }

  for (auto const& attribute : geometry->get_attributes()) {
    pipeline_builder->AddVertexInputAttribute(
      attribute.location, attribute.buffer, attribute.offset,
      attribute.data_type, attribute.components, attribute.normalized);
  }

  return pipeline_builder->Build();
}

void ForwardRenderPipeline::CreateExampleCubeMap(VkCommandBuffer command_buffer) {
  std::shared_ptr<Texture> nx = Texture::load("env/nx.png");
  std::shared_ptr<Texture> ny = Texture::load("env/ny.png");
  std::shared_ptr<Texture> nz = Texture::load("env/nz.png");
  std::shared_ptr<Texture> px = Texture::load("env/px.png");
  std::shared_ptr<Texture> py = Texture::load("env/py.png");
  std::shared_ptr<Texture> pz = Texture::load("env/pz.png");

  std::array<std::shared_ptr<Texture>, 6> textures{ px, nx, py, ny, pz, nz };

  UploadTextureCube(command_buffer, textures);

  cubemap_handle = textures[0].get();
}

void ForwardRenderPipeline::RenderObject(
  std::array<std::unique_ptr<CommandBuffer>, 2>& command_buffers,
  GameObject* object,
  Mesh* mesh
) {
  auto& geometry = mesh->geometry;
  auto& material = mesh->material;

  auto& geometry_data = geometry_cache->Get(geometry);

  auto& object_data = object_cache[object];

  if (!object_data.valid) {
    object_data.bind_group = bind_group_layout->Instantiate();
    object_data.bind_group->Bind(0, camera_data.ubo, BindGroupLayout::Entry::Type::UniformBuffer);

    // Create some dummy uniform buffer and bind it to the bind group
    object_data.ubo = render_device->CreateBuffer(Buffer::Usage::UniformBuffer, sizeof(Matrix4));
    object_data.bind_group->Bind(1, object_data.ubo, BindGroupLayout::Entry::Type::UniformBuffer);

    // Create shader modules
    auto program_key = ProgramKey{typeid(*material), material->get_compile_options()};
    if (program_cache.find(program_key) == program_cache.end()) {
      CompileShaderProgram(material);
    }
    auto& program_data = program_cache[program_key];

    // Create pipeline
    object_data.pipeline = CreatePipeline(
      geometry,
      material,
      pipeline_layout,
      program_data.shader_vert,
      program_data.shader_frag
    );

    object_data.valid = true;
  }

  // Update object transform UBO
  object_data.ubo->Update(&object->transform().world());

  auto texture_slots = material->get_texture_slots();

  for (int i = 0; i < texture_slots.size(); i++) {
    auto& texture = texture_slots[i];
    if (texture) {
      auto match = texture_cache.find(texture.get());

      if (match == texture_cache.end()) {
        UploadTexture((VkCommandBuffer)command_buffers[0]->Handle(), texture);
        match = texture_cache.find(texture.get());
      }

      auto& data = match->second;

      object_data.bind_group->Bind(3 + i, data.texture, data.sampler, GPUTexture::Layout::ShaderReadOnly);
    }
  }

  auto& cube_entry = texture_cache[cubemap_handle];
  object_data.bind_group->Bind(34, cube_entry.texture, cube_entry.sampler, GPUTexture::Layout::ShaderReadOnly);

  // Update and bind material UBO
  auto& uniforms = material->get_uniforms();
  if (material_ubo.find(material.get()) == material_ubo.end()) {
    material_ubo[material.get()] = render_device->CreateBuffer(Buffer::Usage::UniformBuffer, uniforms.size());
  }
  auto& ubo = material_ubo[material.get()];
  ubo->Update(uniforms.data(), uniforms.size());
  object_data.bind_group->Bind(2, ubo, BindGroupLayout::Entry::Type::UniformBuffer);

  auto& index_buffer = geometry->get_index_buffer();

  command_buffers[1]->BindGraphicsPipeline(object_data.pipeline);
  command_buffers[1]->BindGraphicsBindGroup(0, pipeline_layout, object_data.bind_group);
  command_buffers[1]->BindIndexBuffer(geometry_data.ibo, index_buffer->data_type());
  command_buffers[1]->BindVertexBuffers(ArrayView<std::shared_ptr<Buffer>>{
    (std::shared_ptr<Buffer>*)geometry_data.vbos.data(), geometry_data.vbos.size()});

  switch (index_buffer->data_type()) {
    case IndexDataType::UInt16:
      command_buffers[1]->DrawIndexed(index_buffer->size() / sizeof(u16));
      break;
    case IndexDataType::UInt32:
      command_buffers[1]->DrawIndexed(index_buffer->size() / sizeof(u32));
      break;
  }
}

bool ForwardRenderPipeline::IsObjectInsideCameraFrustum(Matrix4 const& modelview, std::shared_ptr<Geometry> const& geometry) {
  auto aabb = geometry->get_bounding_box().ApplyMatrix(modelview);

  return camera_data.frustum->contains_box(aabb);
}

void ForwardRenderPipeline::UpdateCamera(GameObject* camera) {
  // TODO: add support for filters and component inheritance to our component system.  
  if (camera->has_component<PerspectiveCamera>()) {
    auto camera_component = camera->get_component<PerspectiveCamera>();

    *camera_data.projection = camera_component->get_projection();
    camera_data.frustum = &camera_component->get_frustum();
  } else if (camera->has_component<OrthographicCamera>()) {
    auto camera_component = camera->get_component<OrthographicCamera>();

    *camera_data.projection = camera_component->get_projection();
    camera_data.frustum = &camera_component->get_frustum();
  } else {
    Assert(false, "Renderer: camera does not hold a camera component");
  }

  *camera_data.view = camera->transform().world().inverse();

  camera_data.ubo->Update(camera_data.data.data(), camera_data.data.size());
}

void ForwardRenderPipeline::CompileShaderProgram(AnyPtr<Material> material) {
  auto compiler = shaderc::Compiler{};
  auto options = shaderc::CompileOptions{};

  options.SetOptimizationLevel(shaderc_optimization_level_performance);

  auto compile_options = material->get_compile_options();
  auto& compile_option_names = material->get_compile_option_names();

  for (size_t i = 0; i < compile_option_names.size(); i++) {
    if (compile_options & (1 << i)) {
      options.AddMacroDefinition(compile_option_names[i]);
    }
  }

  auto result_vert = compiler.CompileGlslToSpv(
    material->get_vert_shader(),
    shaderc_shader_kind::shaderc_vertex_shader,
    "main.vert",
    options
  );

  auto status_vert = result_vert.GetCompilationStatus();
  if (status_vert != shaderc_compilation_status_success) {
    Log<Error>("Renderer: failed to compile vertex shader ({}):\n{}", status_vert, result_vert.GetErrorMessage());
  }

  auto result_frag = compiler.CompileGlslToSpv(
    material->get_frag_shader(),
    shaderc_shader_kind::shaderc_fragment_shader,
    "main.frag",
    options
  );

  auto status_frag = result_frag.GetCompilationStatus();
  if (status_frag != shaderc_compilation_status_success) {
    Log<Error>("Renderer: failed to compile fragment shader ({}):\n{}", status_frag, result_frag.GetErrorMessage());
  }

  auto spirv_vert = std::vector<u32>{ result_vert.cbegin(), result_vert.cend() };
  auto spirv_frag = std::vector<u32>{ result_frag.cbegin(), result_frag.cend() };

  auto program_key = ProgramKey{typeid(*material), material->get_compile_options()};
  auto& data = program_cache[program_key];
  data.shader_vert = render_device->CreateShaderModule(spirv_vert.data(), spirv_vert.size() * sizeof(u32));
  data.shader_frag = render_device->CreateShaderModule(spirv_frag.data(), spirv_frag.size() * sizeof(u32));
}

void ForwardRenderPipeline::UploadTexture(
  VkCommandBuffer command_buffer,
  std::shared_ptr<Texture>& texture
) {
  // TODO: select mip count based on width/height.
  int mip_count = 6;

  auto width = texture->width();
  auto height = texture->height();
  auto& data = texture_cache[texture.get()];

  if (mip_count == 1) {
    data.texture = render_device->CreateTexture2D(
      width,
      height,
      GPUTexture::Format::R8G8B8A8_SRGB,
      GPUTexture::Usage::CopyDst | GPUTexture::Usage::Sampled,
      1
    );
  } else {
    data.texture = render_device->CreateTexture2D(
      width,
      height,
      GPUTexture::Format::R8G8B8A8_SRGB,
      GPUTexture::Usage::CopySrc | GPUTexture::Usage::CopyDst | GPUTexture::Usage::Sampled,
      mip_count
    );
  }

  data.buffer = render_device->CreateBufferWithData(
    Buffer::Usage::CopySrc,
    texture->data(),
    sizeof(u32) * width * height
  );

  data.sampler = render_device->CreateSampler({
    .address_mode_u = Sampler::AddressMode::Repeat,
    .address_mode_v = Sampler::AddressMode::Repeat,
    .min_filter = Sampler::FilterMode::Linear,
    .mag_filter = Sampler::FilterMode::Linear,
    .mip_filter = Sampler::FilterMode::Linear,
    .anisotropy = true,
    .max_anisotropy = 16
  });

  auto region = VkBufferImageCopy{
    .bufferOffset = 0,
    .bufferRowLength = width,
    .bufferImageHeight = height,
    .imageSubresource = VkImageSubresourceLayers{
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .mipLevel = 0,
      .baseArrayLayer = 0,
      .layerCount = 1
    },
    .imageOffset = VkOffset3D{
      .x = 0,
      .y = 0,
      .z = 0
    },
    .imageExtent = VkExtent3D{
      .width = width,
      .height = height,
      .depth = 1
    }
  };

  TransitionImageLayout(
    command_buffer,
    data.texture,
    GPUTexture::Layout::Undefined,
    GPUTexture::Layout::CopyDst
  );

  vkCmdCopyBufferToImage(
    command_buffer,
    (VkBuffer)data.buffer->Handle(),
    (VkImage)data.texture->handle2(),
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    1,
    &region
  );

  if (mip_count == 1) {
    TransitionImageLayout(
      command_buffer,
      data.texture,
      GPUTexture::Layout::CopyDst,
      GPUTexture::Layout::ShaderReadOnly
    );
  } else {
    GenerateMipMaps(command_buffer, data.texture);
  }
}

void ForwardRenderPipeline::UploadTextureCube(
  VkCommandBuffer command_buffer,
  std::array<std::shared_ptr<Texture>, 6>& textures
) {
  // TODO: select mip count based on width/height.
  int mip_count = 6;

  // TODO: fix the texture caching.
  auto width = textures[0]->width();
  auto height = textures[0]->height();
  auto& data = texture_cache[textures[0].get()];

  if (mip_count == 1) {
    data.texture = render_device->CreateTextureCube(
      width,
      height,
      GPUTexture::Format::R8G8B8A8_SRGB,
      GPUTexture::Usage::CopyDst | GPUTexture::Usage::Sampled
    );
  } else {
    data.texture = render_device->CreateTextureCube(
      width,
      height,
      GPUTexture::Format::R8G8B8A8_SRGB,
      GPUTexture::Usage::CopySrc | GPUTexture::Usage::CopyDst | GPUTexture::Usage::Sampled,
      mip_count
    );
  }

  auto face_size = sizeof(u32) * width * height;

  data.buffer = render_device->CreateBuffer(Buffer::Usage::CopySrc, face_size * 6);

  for (int i = 0; i < 6; i++) {
    data.buffer->Update(textures[i]->data(), face_size, face_size * i);
  }

  data.sampler = render_device->CreateSampler({
    .address_mode_u = Sampler::AddressMode::Repeat,
    .address_mode_v = Sampler::AddressMode::Repeat,
    .min_filter = Sampler::FilterMode::Linear,
    .mag_filter = Sampler::FilterMode::Linear,
    .mip_filter = Sampler::FilterMode::Linear
  });

  auto region = VkBufferImageCopy{
    .bufferOffset = 0,
    .bufferRowLength = width,
    .bufferImageHeight = height,
    .imageSubresource = VkImageSubresourceLayers{
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .mipLevel = 0,
      .baseArrayLayer = 0,
      .layerCount = 6
    },
    .imageOffset = VkOffset3D{
      .x = 0,
      .y = 0,
      .z = 0
    },
    .imageExtent = VkExtent3D{
      .width = width,
      .height = height,
      .depth = 1
    }
  };

  TransitionImageLayout(
    command_buffer,
    data.texture,
    GPUTexture::Layout::Undefined,
    GPUTexture::Layout::CopyDst
  );

  vkCmdCopyBufferToImage(
    command_buffer,
    (VkBuffer)data.buffer->Handle(),
    (VkImage)data.texture->handle2(),
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    1,
    &region
  );

  if (mip_count == 1) {
    TransitionImageLayout(
      command_buffer,
      data.texture,
      GPUTexture::Layout::CopyDst,
      GPUTexture::Layout::ShaderReadOnly
    );
  } else {
    GenerateMipMaps(command_buffer, data.texture);
  }
}

void ForwardRenderPipeline::GenerateMipMaps(VkCommandBuffer command_buffer, AnyPtr<GPUTexture> texture) {
  const int mip_count = texture->mip_levels();

  TransitionImageLayout(
    command_buffer,
    texture,
    GPUTexture::Layout::CopyDst,
    GPUTexture::Layout::CopySrc,
    0
  );

  auto mip_width = texture->width();
  auto mip_height = texture->height();

  for (int i = 1; i < mip_count; i++) {
    auto blit = VkImageBlit{
      .srcSubresource = VkImageSubresourceLayers{
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .mipLevel = (u32)(i - 1),
        .baseArrayLayer = 0,
        .layerCount = texture->layers()
      },
      .dstSubresource = VkImageSubresourceLayers{
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .mipLevel = (u32)i,
        .baseArrayLayer = 0,
        .layerCount = texture->layers()
      }
    };

    blit.srcOffsets[0] = { .x = 0, .y = 0, .z = 0 };
    blit.srcOffsets[1] = { .x = (s32)mip_width, .y = (s32)mip_height, .z = 1 };

    if (mip_width > 1) mip_width /= 2;
    if (mip_height > 1) mip_height /= 2;

    blit.dstOffsets[0] = { .x = 0, .y = 0, .z = 0 };
    blit.dstOffsets[1] = { .x = (s32)mip_width, .y = (s32)mip_height, .z = 1 };

    TransitionImageLayout(
      command_buffer,
      texture,
      GPUTexture::Layout::Undefined,
      GPUTexture::Layout::CopyDst,
      i
    );

    // TODO: we might want to use a cubic filter if available!
    vkCmdBlitImage(
      command_buffer,
      (VkImage)texture->handle2(),
      VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
      (VkImage)texture->handle2(),
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      1, &blit,
      VK_FILTER_LINEAR
    );

    // TODO: this is sort of redundant for the last image.
    TransitionImageLayout(
      command_buffer,
      texture,
      GPUTexture::Layout::CopyDst,
      GPUTexture::Layout::CopySrc,
      i
    );
  }

  TransitionImageLayout(
    command_buffer,
    texture,
    GPUTexture::Layout::CopySrc,
    GPUTexture::Layout::ShaderReadOnly,
    0,
    mip_count
  );
}

void ForwardRenderPipeline::TransitionImageLayout(
  VkCommandBuffer command_buffer,
  AnyPtr<GPUTexture> texture,
  GPUTexture::Layout old_layout,
  GPUTexture::Layout new_layout,
  u32 base_mip,
  u32 mip_count
) {
  auto barrier = VkImageMemoryBarrier{
    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
    .srcAccessMask = 0,
    .dstAccessMask = 0,
    .oldLayout = (VkImageLayout)old_layout,
    .newLayout = (VkImageLayout)new_layout,
    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
    .image = (VkImage)texture->handle2(),
    .subresourceRange = VkImageSubresourceRange{
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .baseMipLevel = base_mip,
      .levelCount = mip_count,
      .baseArrayLayer = 0,
      .layerCount = texture->layers()
    }
  };

  auto src_stage = VkPipelineStageFlags{};
  auto dst_stage = VkPipelineStageFlags{};

  if (old_layout == GPUTexture::Layout::Undefined && new_layout == GPUTexture::Layout::CopyDst) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    dst_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
  } else if (old_layout == GPUTexture::Layout::CopyDst && new_layout == GPUTexture::Layout::CopySrc) {
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

    // TODO: make sure that this is actually valid and correct.
    src_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    dst_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
  } else if (old_layout == GPUTexture::Layout::CopySrc && new_layout == GPUTexture::Layout::ShaderReadOnly) {
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    // TODO: what if a texture is read from the vertex shader?
    src_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    dst_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  } else if (old_layout == GPUTexture::Layout::CopyDst && new_layout == GPUTexture::Layout::ShaderReadOnly) {
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    // TODO: what if a texture is read from the vertex shader?
    src_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    dst_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  } else if (old_layout == GPUTexture::Layout::ColorAttachment && new_layout == GPUTexture::Layout::ShaderReadOnly) {
    barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    // TODO: what if a texture is read from the vertex shader?
    src_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dst_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  } else {
    Assert(false, "Vulkan: unhandled image layout transition: {} to {}", (u32)old_layout, (u32)new_layout);
  }

  vkCmdPipelineBarrier(
    command_buffer,
    src_stage,
    dst_stage,
    0,
    0, nullptr,
    0, nullptr,
    1, &barrier
  );
}

} // namespace Aura
