/*
 * Copyright (C) 2022 fleroviux
 */

#include <shaderc/shaderc.hpp>

#include "renderer.hpp"

// TODO: remove this hacky include
#include "../../gal/src/vulkan/render_pass.hpp"

namespace Aura {

void Renderer::Initialize(
  VkPhysicalDevice physical_device,
  VkDevice device,
  std::shared_ptr<RenderDevice> render_device
) {
  this->physical_device = physical_device;
  this->device = device;
  this->render_device = render_device;
  CreateCameraUniformBlock();
  CreateRenderTarget();
}

void Renderer::Render(
  std::array<VkCommandBuffer, 2>& command_buffers,
  GameObject* scene
) {
  const std::function<void(GameObject*)> traverse = [&](GameObject* object) {
    if (!object->visible()) {
      return;
    }

    // Update object transform:
    // TODO: we possibly do not update the camera transform in time.
    auto& transform = object->transform();
    if (transform.auto_update()) {
      transform.update_local();
    }
    transform.update_world(false);

    auto mesh = object->get_component<Mesh>();

    if (mesh && mesh->visible) {
      RenderObject(command_buffers, object, mesh);
    }

    for (auto child : object->children()) traverse(child);
  };

  // TODO: verify that scene component exists and camera is non-null.
  UpdateCameraUniformBlock(scene->get_component<Scene>()->camera);

  auto render_pass_ = (VulkanRenderPass*)render_pass.get();
  auto& clear_values = render_pass_->GetClearValues();
  auto render_pass_begin_info = VkRenderPassBeginInfo{
    .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
    .pNext = nullptr,
    .renderPass = render_pass_->Handle(),
    .framebuffer = (VkFramebuffer)render_target->handle(),
    .renderArea = VkRect2D{
      .offset = VkOffset2D{
        .x = 0,
        .y = 0
      },
      .extent = VkExtent2D{
        .width = 1600,
        .height = 900
      }
    },
    .clearValueCount = (u32)clear_values.size(),
    .pClearValues = clear_values.data()
  };

  vkCmdBeginRenderPass(
    command_buffers[1],
    &render_pass_begin_info,
    VK_SUBPASS_CONTENTS_INLINE
  );

  traverse(scene);

  vkCmdEndRenderPass(command_buffers[1]);

  // TODO: use a subpass dependency to transition image layout.
  TransitionImageLayout(
    command_buffers[1],
    color_texture,
    GPUTexture::Layout::ColorAttachment,
    GPUTexture::Layout::ShaderReadOnly
  );
}

void Renderer::RenderObject(
  std::array<VkCommandBuffer, 2>& command_buffers,
  GameObject* object,
  Mesh* mesh
) {
  auto& geometry = mesh->geometry;
  auto& material = mesh->material;

  auto& object_data = object_cache[object];

  if (!object_data.valid) {
    // Create bind group layout, pipeline layout and bind group.
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
    object_data.bind_group_layout = render_device->CreateBindGroupLayout(bindings);
    object_data.pipeline_layout = render_device->CreatePipelineLayout({ object_data.bind_group_layout });
    object_data.bind_group = object_data.bind_group_layout->Instantiate();
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
      object_data.pipeline_layout,
      program_data.shader_vert,
      program_data.shader_frag
    );

    object_data.valid = true;
  }

  if (geometry_cache.find(geometry.get()) == geometry_cache.end()) {
    auto& geometry_data = geometry_cache[geometry.get()];

    // Upload index buffer
    geometry_data.ibo = render_device->CreateBufferWithData(
      Buffer::Usage::IndexBuffer,
      geometry->index_buffer.view<u8>()
    );

    // Upload vertex buffers
    for (auto const& buffer : geometry->buffers) {
      geometry_data.vbos.push_back(render_device->CreateBufferWithData(
        Buffer::Usage::VertexBuffer,
        buffer.view<u8>()
      ));
    }
  }

  auto& geometry_data = geometry_cache[geometry.get()];

  // Update object transform UBO
  object_data.ubo->Update(&object->transform().world());

  auto texture_slots = material->get_texture_slots();

  for (int i = 0; i < texture_slots.size(); i++) {
    auto& texture = texture_slots[i];
    if (texture) {
      auto match = texture_cache.find(texture.get());

      if (match == texture_cache.end()) {
        UploadTexture(command_buffers[0], texture);
        match = texture_cache.find(texture.get());
      }

      auto& data = match->second;

      object_data.bind_group->Bind(3 + i, data.texture, data.sampler);
    }
  }

  // Update and bind material UBO
  auto& uniforms = material->get_uniforms();
  if (material_ubo.find(material.get()) == material_ubo.end()) {
    material_ubo[material.get()] = render_device->CreateBuffer(Buffer::Usage::UniformBuffer, uniforms.size());
  }
  auto& ubo = material_ubo[material.get()];
  ubo->Update(uniforms.data(), uniforms.size());
  object_data.bind_group->Bind(2, ubo, BindGroupLayout::Entry::Type::UniformBuffer);

  // TODO: creating a std::vector everytime is slow. make this faster.
  auto descriptor_set = (VkDescriptorSet)object_data.bind_group->Handle();
  auto vbo_handles = std::vector<VkBuffer>{};
  auto vbo_offsets = std::vector<VkDeviceSize>{};
  for (auto& vbo : geometry_data.vbos) {
    vbo_handles.push_back((VkBuffer)vbo->Handle());
    vbo_offsets.push_back(0);
  }
  vkCmdBindPipeline(command_buffers[1], VK_PIPELINE_BIND_POINT_GRAPHICS, object_data.pipeline);
  vkCmdBindVertexBuffers(command_buffers[1], 0, vbo_handles.size(), vbo_handles.data(), vbo_offsets.data());
  vkCmdBindDescriptorSets(
    command_buffers[1],
    VK_PIPELINE_BIND_POINT_GRAPHICS,
    (VkPipelineLayout)object_data.pipeline_layout->Handle(),
    0,
    1,
    &descriptor_set,
    0,
    nullptr
  );

  auto& index_buffer = geometry->index_buffer;

  auto index_type = VkIndexType{};
  auto index_count = 0UL;

  if (index_buffer.data_type() == IndexDataType::UInt16) {
    index_type = VK_INDEX_TYPE_UINT16;
    index_count = index_buffer.size() / sizeof(u16);
  } else {
    index_type = VK_INDEX_TYPE_UINT32;
    index_count = index_buffer.size() / sizeof(u32);
  }

  vkCmdBindIndexBuffer(
    command_buffers[1],
    (VkBuffer)geometry_data.ibo->Handle(),
    0,
    index_type
  );

  vkCmdDrawIndexed(command_buffers[1], index_count, 1, 0, 0, 0);
}

void Renderer::CompileShaderProgram(std::shared_ptr<Material>& material) {
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

void Renderer::UploadTexture(
  VkCommandBuffer command_buffer,
  std::shared_ptr<Texture>& texture
) {
  auto width = texture->width();
  auto height = texture->height();
  auto& data = texture_cache[texture.get()];

  data.texture = render_device->CreateTexture2D(
    width,
    height,
    GPUTexture::Format::R8G8B8A8_SRGB,
    GPUTexture::Usage::CopyDst | GPUTexture::Usage::Sampled
  );

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

  TransitionImageLayout(
    command_buffer,
    data.texture,
    GPUTexture::Layout::CopyDst,
    GPUTexture::Layout::ShaderReadOnly
  );
}

void Renderer::TransitionImageLayout(
  VkCommandBuffer command_buffer,
  AnyPtr<GPUTexture> texture,
  GPUTexture::Layout old_layout,
  GPUTexture::Layout new_layout
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
      .baseMipLevel = 0,
      .levelCount = 1,
      .baseArrayLayer = 0,
      .layerCount = 1
    }
  };

  auto src_stage = VkPipelineStageFlags{};
  auto dst_stage = VkPipelineStageFlags{};

  if (old_layout == GPUTexture::Layout::Undefined && new_layout == GPUTexture::Layout::CopyDst) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    dst_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
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

void Renderer::CreateCameraUniformBlock() {
  auto layout = UniformBlockLayout{};
  layout.add<Matrix4>("projection");
  layout.add<Matrix4>("view");

  camera_data.data = UniformBlock{ layout };
  camera_data.projection = &camera_data.data.get<Matrix4>("projection");
  camera_data.view = &camera_data.data.get<Matrix4>("view");
  camera_data.ubo = render_device->CreateBuffer(Buffer::Usage::UniformBuffer, camera_data.data.size());
}

void Renderer::UpdateCameraUniformBlock(GameObject* camera) {
  if (camera->has_component<PerspectiveCamera>()) {
    // TODO: switch to using an OpenGL style projection matrix?
    auto cam = camera->get_component<PerspectiveCamera>();
    *camera_data.projection = Matrix4::perspective_dx(
      cam->field_of_view, cam->aspect_ratio, cam->near, cam->far);
  } else if (camera->has_component<OrthographicCamera>()) {
    Assert(false, "Renderer: orthographic camera not supported");
  } else {
    Assert(false, "Renderer: camera does not hold a camera component");
  }

  // TODO: optimize the camera view matrix calculation.
  *camera_data.view = camera->transform().world().inverse();

  camera_data.ubo->Update(camera_data.data.data(), camera_data.data.size());
}

void Renderer::CreateRenderTarget() {
  color_texture = render_device->CreateTexture2D(
    1600,
    900,
    GPUTexture::Format::B8G8R8A8_SRGB,
    GPUTexture::Usage::ColorAttachment | GPUTexture::Usage::Sampled
  );

  depth_texture = render_device->CreateTexture2D(
    1600,
    900,
    GPUTexture::Format::DEPTH_F32,
    GPUTexture::Usage::DepthStencilAttachment
  );

  render_target = render_device->CreateRenderTarget({ color_texture }, depth_texture);
  render_pass = render_target->CreateRenderPass();

  render_pass->SetClearColor(0, 0.01, 0.01, 0.01, 1);
  render_pass->SetClearDepth(1); 
}

auto Renderer::CreatePipeline(
  std::shared_ptr<Geometry>& geometry,
  std::unique_ptr<PipelineLayout>& pipeline_layout,
  std::unique_ptr<ShaderModule>& shader_vert,
  std::unique_ptr<ShaderModule>& shader_frag
) -> VkPipeline {
  auto pipeline = VkPipeline{};

  VkPipelineShaderStageCreateInfo pipeline_stages[] = {
    {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .stage = VK_SHADER_STAGE_VERTEX_BIT,
      .module = (VkShaderModule)shader_vert->Handle(),
      .pName = "main",
      .pSpecializationInfo = nullptr
    },
    {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
      .module = (VkShaderModule)shader_frag->Handle(),
      .pName = "main",
      .pSpecializationInfo = nullptr
    }
  };

  auto viewport = VkViewport{
    .x = 0,
    .y = 900,
    .width = 1600,
    .height = -900,
    .minDepth = 0,
    .maxDepth = 1
  };

  auto scissor = VkRect2D{
    .offset = VkOffset2D{
      .x = 0,
      .y = 0
    },
    .extent = VkExtent2D{
      .width = 1600,
      .height = 900
    }
  };

  // TODO: use dynamic viewport and scissor state?
  auto viewport_info = VkPipelineViewportStateCreateInfo{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
    .pNext = nullptr,
    .flags = 0,
    .viewportCount = 1,
    .pViewports = &viewport,
    .scissorCount = 1,
    .pScissors = &scissor
  };

  auto rasterization_info = VkPipelineRasterizationStateCreateInfo{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
    .pNext = nullptr,
    .flags = 0,
    .depthClampEnable = VK_FALSE,
    .rasterizerDiscardEnable = VK_FALSE,
    .polygonMode = VK_POLYGON_MODE_FILL,
    .cullMode = VK_CULL_MODE_NONE,
    .depthBiasEnable = VK_FALSE,
    .depthBiasConstantFactor = 0,
    .depthBiasClamp = 0,
    .depthBiasSlopeFactor = 0,
    .lineWidth = 1
  };

  auto multisample_info = VkPipelineMultisampleStateCreateInfo{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
    .pNext = nullptr,
    .flags = 0,
    .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
    .sampleShadingEnable = VK_FALSE,
    .minSampleShading = 0,
    .pSampleMask = nullptr,
    .alphaToCoverageEnable = VK_FALSE,
    .alphaToOneEnable = VK_FALSE
  };

  auto depth_stencil_info = VkPipelineDepthStencilStateCreateInfo{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
    .pNext = nullptr,
    .flags = 0,
    .depthTestEnable = VK_TRUE,
    .depthWriteEnable = VK_TRUE,
    .depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
    .depthBoundsTestEnable = VK_FALSE,
    .stencilTestEnable = VK_FALSE,
    .front = {},
    .back = {},
    .minDepthBounds = 0,
    .maxDepthBounds = 0
  };

  auto color_blend_attachment_info = VkPipelineColorBlendAttachmentState{
    .blendEnable = VK_FALSE,
    .colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                      VK_COLOR_COMPONENT_G_BIT |
                      VK_COLOR_COMPONENT_B_BIT |
                      VK_COLOR_COMPONENT_A_BIT
  };

  auto color_blend_info = VkPipelineColorBlendStateCreateInfo{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
    .pNext = nullptr,
    .flags = 0,
    .logicOpEnable = VK_FALSE,
    .logicOp = VK_LOGIC_OP_NO_OP,
    .attachmentCount = 1,
    .pAttachments = &color_blend_attachment_info,
    .blendConstants = {0, 0, 0, 0}
  };

  auto bindings = std::vector<VkVertexInputBindingDescription>{};
  auto attributes = std::vector<VkVertexInputAttributeDescription>{};
  BuildPipelineGeometryInfo(geometry, bindings, attributes);

  auto vertex_input_state_info = VkPipelineVertexInputStateCreateInfo{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
    .pNext = nullptr,
    .flags = 0,
    .vertexBindingDescriptionCount = u32(bindings.size()),
    .pVertexBindingDescriptions = bindings.data(),
    .vertexAttributeDescriptionCount = u32(attributes.size()),
    .pVertexAttributeDescriptions = attributes.data()
  };

  auto input_assembly_state_info = VkPipelineInputAssemblyStateCreateInfo{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
    .pNext = nullptr,
    .flags = 0,
    .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
    .primitiveRestartEnable = VK_FALSE
  };

  auto pipeline_info = VkGraphicsPipelineCreateInfo{
    .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
    .pNext = nullptr,
    .flags = 0,
    .stageCount = 2,
    .pStages = pipeline_stages,
    .pVertexInputState = &vertex_input_state_info,
    .pInputAssemblyState = &input_assembly_state_info,
    .pTessellationState = nullptr,
    .pViewportState = &viewport_info,
    .pRasterizationState = &rasterization_info,
    .pMultisampleState = &multisample_info,
    .pDepthStencilState = &depth_stencil_info,
    .pColorBlendState = &color_blend_info,
    .pDynamicState = nullptr,
    .layout = (VkPipelineLayout)pipeline_layout->Handle(),
    .renderPass = ((VulkanRenderPass*)render_pass.get())->Handle(),
    .subpass = 0,
    .basePipelineHandle = VK_NULL_HANDLE,
    .basePipelineIndex = 0
  };

  if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &pipeline) != VK_SUCCESS) {
    Assert(false, "Renderer: failed to create graphics pipeline :(");
  }

  return pipeline;
}

void Renderer::BuildPipelineGeometryInfo(
  std::shared_ptr<Geometry>& geometry,
  std::vector<VkVertexInputBindingDescription>& bindings,
  std::vector<VkVertexInputAttributeDescription>& attributes
) {
  for (auto& buffer : geometry->buffers) {
    auto& layout = buffer.layout();
    auto binding = u32(bindings.size());

    bindings.push_back(VkVertexInputBindingDescription{
      .binding = binding,
      .stride = u32(layout.stride),
      .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
    });

    for (auto& attribute : layout.attributes) {
      attributes.push_back(VkVertexInputAttributeDescription{
        .location = u32(attribute.index),
        .binding = binding,
        .format = GetVkFormatFromAttribute(attribute),
        .offset = u32(attribute.offset)
      });
    }
  }
}

auto Renderer::GetVkFormatFromAttribute(
  VertexBufferLayout::Attribute const& attribute
) -> VkFormat {
  const auto components = attribute.components;
  const bool normalized = attribute.normalized;

  switch (attribute.data_type) {
    case VertexDataType::SInt8: {
      if (normalized) {
        if (components == 1) return VK_FORMAT_R8_SNORM;
        if (components == 2) return VK_FORMAT_R8G8_SNORM;
        if (components == 3) return VK_FORMAT_R8G8B8_SNORM;
        if (components == 4) return VK_FORMAT_R8G8B8A8_SNORM;
      } else {
        if (components == 1) return VK_FORMAT_R8_SINT;
        if (components == 2) return VK_FORMAT_R8G8_SINT;
        if (components == 3) return VK_FORMAT_R8G8B8_SINT;
        if (components == 4) return VK_FORMAT_R8G8B8A8_SINT; 
      } 
      break;
    }
    case VertexDataType::UInt8: {
      if (normalized) {
        if (components == 1) return VK_FORMAT_R8_UNORM;
        if (components == 2) return VK_FORMAT_R8G8_UNORM;
        if (components == 3) return VK_FORMAT_R8G8B8_UNORM;
        if (components == 4) return VK_FORMAT_R8G8B8A8_UNORM;
      } else {
        if (components == 1) return VK_FORMAT_R8_UINT;
        if (components == 2) return VK_FORMAT_R8G8_UINT;
        if (components == 3) return VK_FORMAT_R8G8B8_UINT;
        if (components == 4) return VK_FORMAT_R8G8B8A8_UINT;
      }
      break;
    }
    case VertexDataType::SInt16: {
      if (normalized) {
        if (components == 1) return VK_FORMAT_R16_SNORM;
        if (components == 2) return VK_FORMAT_R16G16_SNORM;
        if (components == 3) return VK_FORMAT_R16G16B16_SNORM;
        if (components == 4) return VK_FORMAT_R16G16B16A16_SNORM;
      } else {
        if (components == 1) return VK_FORMAT_R16_SINT;
        if (components == 2) return VK_FORMAT_R16G16_SINT;
        if (components == 3) return VK_FORMAT_R16G16B16_SINT;
        if (components == 4) return VK_FORMAT_R16G16B16A16_SINT;
      }
      break;
    }
    case VertexDataType::UInt16: {
      if (normalized) {
        if (components == 1) return VK_FORMAT_R16_UNORM;
        if (components == 2) return VK_FORMAT_R16G16_UNORM;
        if (components == 3) return VK_FORMAT_R16G16B16_UNORM;
        if (components == 4) return VK_FORMAT_R16G16B16A16_UNORM;
      } else {
        if (components == 1) return VK_FORMAT_R16_UINT;
        if (components == 2) return VK_FORMAT_R16G16_UINT;
        if (components == 3) return VK_FORMAT_R16G16B16_UINT;
        if (components == 4) return VK_FORMAT_R16G16B16A16_UINT;
      }
      break;
    }
    case VertexDataType::Float16: {
      if (components == 1) return VK_FORMAT_R16_SFLOAT;
      if (components == 2) return VK_FORMAT_R16G16_SFLOAT;
      if (components == 3) return VK_FORMAT_R16G16B16_SFLOAT;
      if (components == 4) return VK_FORMAT_R16G16B16A16_SFLOAT;
      break;
    }
    case VertexDataType::Float32: {
      if (components == 1) return VK_FORMAT_R32_SFLOAT;
      if (components == 2) return VK_FORMAT_R32G32_SFLOAT;
      if (components == 3) return VK_FORMAT_R32G32B32_SFLOAT;
      if (components == 4) return VK_FORMAT_R32G32B32A32_SFLOAT;
      break;
    }
  }

  Assert(false, "Renderer: failed to map vertex attribute to VkFormat");
}

} // namespace Aura
