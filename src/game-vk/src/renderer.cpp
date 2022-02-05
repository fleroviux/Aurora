/*
 * Copyright (C) 2022 fleroviux
 */

#include "renderer.hpp"

// Include test shaders
#include "shader/surface.vert.h"
#include "shader/surface.frag.h"

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
  CreateRenderTarget();
}

void Renderer::Render(VkCommandBuffer command_buffer, GameObject* scene) {
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
      RenderObject(command_buffer, object, mesh);
    }

    for (auto child : object->children()) traverse(child);
  };

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
    command_buffer,
    &render_pass_begin_info,
    VK_SUBPASS_CONTENTS_INLINE
  );

  traverse(scene);

  vkCmdEndRenderPass(command_buffer);
}

void Renderer::RenderObject(
  VkCommandBuffer command_buffer,
  GameObject* object,
  Mesh* mesh
) {
  auto& object_data = object_cache[object];

  if (!object_data.valid) {
    auto& geometry = mesh->geometry;

    // Create bind group layout, pipeline layout and bind group.
    object_data.bind_group_layout = render_device->CreateBindGroupLayout({
      {
        .binding = 0,
        .type = BindGroupLayout::Entry::Type::UniformBuffer
      }
    });
    object_data.pipeline_layout = render_device->CreatePipelineLayout({ object_data.bind_group_layout });
    object_data.bind_group = object_data.bind_group_layout->Instantiate();

    // Create some dummy uniform buffer and bind it to the bind group
    object_data.ubo = render_device->CreateBuffer(Buffer::Usage::UniformBuffer, sizeof(Matrix4));
    object_data.bind_group->Bind(0, object_data.ubo, BindGroupLayout::Entry::Type::UniformBuffer);

    // Create shader modules
    object_data.shader_vert = render_device->CreateShaderModule(surface_vert, sizeof(surface_vert));
    object_data.shader_frag = render_device->CreateShaderModule(surface_frag, sizeof(surface_frag));

    // Upload index buffer
    object_data.ibo = render_device->CreateBufferWithData(
      Buffer::Usage::IndexBuffer,
      geometry->index_buffer.view<u8>()
    );

    // Upload vertex buffers
    for (auto const& buffer : geometry->buffers) {
      object_data.vbos.push_back(render_device->CreateBufferWithData(
        Buffer::Usage::VertexBuffer,
        buffer.view<u8>()
      ));
    }

    // Create pipeline
    object_data.pipeline = CreatePipeline(
      geometry,
      object_data.pipeline_layout,
      object_data.shader_vert,
      object_data.shader_frag
    );

    object_data.valid = true;
  }

  // Update object transform UBO
  auto projection = Matrix4::perspective_dx(45 / 180.0 * 3.141592, 1600.0 / 900, 0.01, 100.0);
  auto transform = projection * object->transform().world();
  object_data.ubo->Update(&transform);

  // TODO: creating a std::vector everytime is slow. make this faster.
  auto descriptor_set = (VkDescriptorSet)object_data.bind_group->Handle();
  auto vbo_handles = std::vector<VkBuffer>{};
  auto vbo_offsets = std::vector<VkDeviceSize>{};
  for (auto& vbo : object_data.vbos) {
    vbo_handles.push_back((VkBuffer)vbo->Handle());
    vbo_offsets.push_back(0);
  }
  vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, object_data.pipeline);
  vkCmdBindVertexBuffers(command_buffer, 0, vbo_handles.size(), vbo_handles.data(), vbo_offsets.data());
  vkCmdBindDescriptorSets(
    command_buffer,
    VK_PIPELINE_BIND_POINT_GRAPHICS,
    (VkPipelineLayout)object_data.pipeline_layout->Handle(),
    0,
    1,
    &descriptor_set,
    0,
    nullptr
  );

  auto& index_buffer = mesh->geometry->index_buffer;

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
    command_buffer,
    (VkBuffer)object_data.ibo->Handle(),
    0,
    index_type
  );

  vkCmdDrawIndexed(command_buffer, index_count, 1, 0, 0, 0);
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

  render_pass->SetClearColor(0, 1, 0, 0, 1);
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
