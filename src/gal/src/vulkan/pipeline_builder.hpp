/*
 * Copyright (C) 2022 fleroviux
 */

#pragma once

#include <aurora/gal/backend/vulkan.hpp>
#include <aurora/log.hpp>
#include <vector>

namespace Aura {

struct VulkanGraphicsPipeline final : GraphicsPipeline {
  struct Own {
    std::shared_ptr<ShaderModule> shader_vert;
    std::shared_ptr<ShaderModule> shader_frag;
    std::shared_ptr<PipelineLayout> layout;
    std::shared_ptr<RenderPass> render_pass;
  };

  VulkanGraphicsPipeline(
    VkDevice device,
    VkPipeline pipeline,
    Own own
  )   : device(device), pipeline(pipeline), own(own) {
  }

 ~VulkanGraphicsPipeline() {
    vkDestroyPipeline(device, pipeline, nullptr);
  }

  auto Handle() -> void* override {
    return (void*)pipeline;
  }

private:
  VkDevice device;
  VkPipeline pipeline;
  Own own;
};

struct VulkanGraphicsPipelineBuilder final : GraphicsPipelineBuilder {
  VulkanGraphicsPipelineBuilder(VkDevice device) : device(device) {
  }

  void SetViewport(int x, int y, int width, int height) override {
    viewport.x = (float)x;
    viewport.y = (float)y;
    viewport.width = (float)width;
    viewport.height = (float)height;

    if (!have_explicit_scissor) {
      SetScissorInternal(x, y, width, height);
    }
  }

  void SetScissor(int x, int y, int width, int height) override {
    SetScissorInternal(x, y, width, height);

    have_explicit_scissor = true;
  }

  void SetShaderModule(PipelineStage stage, std::shared_ptr<ShaderModule> shader_module) override {
    switch (stage) {
      case PipelineStage::Vertex:
        own.shader_vert = shader_module;
        pipeline_stages[0].module = (VkShaderModule)shader_module->Handle();
        break;
      case PipelineStage::Fragment:
        own.shader_frag = shader_module;
        pipeline_stages[1].module = (VkShaderModule)shader_module->Handle();
        break;
    }
  }

  void SetPipelineLayout(std::shared_ptr<PipelineLayout> layout) override {
    own.layout = layout;
    pipeline_info.layout = (VkPipelineLayout)layout->Handle();
  }

  void SetRenderPass(std::shared_ptr<RenderPass> render_pass) override {
    own.render_pass = render_pass;
    pipeline_info.renderPass = ((VulkanRenderPass*)(render_pass.get()))->Handle();
  }

  void SetRasterizerDiscardEnable(bool enable) override {
    rasterization_info.rasterizerDiscardEnable = enable ? VK_TRUE : VK_FALSE;
  }

  void SetPolygonMode(PolygonMode mode) override {
    rasterization_info.polygonMode = (VkPolygonMode)mode;
  }

  void SetPolygonCull(PolygonFace face) override {
    rasterization_info.cullMode = (VkCullModeFlags)face;
  }

  void SetLineWidth(float width) override {
    rasterization_info.lineWidth = width;
  }

  void SetDepthTestEnable(bool enable) override {
    depth_stencil_info.depthTestEnable = enable ? VK_TRUE : VK_FALSE;
  }

  void SetDepthWriteEnable(bool enable) override {
    depth_stencil_info.depthWriteEnable = enable ? VK_TRUE : VK_FALSE;
  }

  void SetDepthCompareOp(CompareOp compare_op) override {
    depth_stencil_info.depthCompareOp = (VkCompareOp)compare_op;
  }

  void SetPrimitiveTopology(PrimitiveTopology topology) override {
    input_assembly_info.topology = (VkPrimitiveTopology)topology;
  }

  void SetPrimitiveRestartEnable(bool enable) override {
    input_assembly_info.primitiveRestartEnable = enable ? VK_TRUE : VK_FALSE;
  }

  void ResetVertexInput() override {
    vertex_input_bindings.clear();
    vertex_input_attributes.clear();
  }

  void AddVertexInputBinding(u32 binding, u32 stride, VertexInputRate input_rate) override {
    vertex_input_bindings.push_back({
      .binding = binding,
      .stride = stride,
      .inputRate = (VkVertexInputRate)input_rate
    });
  }

  void AddVertexInputAttribute(
    u32 location,
    u32 binding,
    u32 offset,
    VertexDataType data_type,
    int components,
    bool normalized
  ) override {
    vertex_input_attributes.push_back({
      .location = location,
      .binding = binding,
      .format = GetVertexInputAttributeFormat(data_type, components, normalized),
      .offset = offset
    });
  }

  void SetBlendEnable(size_t color_attachment, bool enable) override {
    CheckColorAttachmentIndex(color_attachment);
    attachment_blend_state[color_attachment].blendEnable = enable ? VK_TRUE : VK_FALSE;
  }

  void SetSrcColorBlendFactor(size_t color_attachment, BlendFactor blend_factor) override {
    CheckColorAttachmentIndex(color_attachment);
    attachment_blend_state[color_attachment].srcColorBlendFactor = (VkBlendFactor)blend_factor;
  }

  void SetSrcAlphaBlendFactor(size_t color_attachment, BlendFactor blend_factor) override {
    CheckColorAttachmentIndex(color_attachment);
    attachment_blend_state[color_attachment].srcAlphaBlendFactor = (VkBlendFactor)blend_factor;
  }

  void SetDstColorBlendFactor(size_t color_attachment, BlendFactor blend_factor) override {
    CheckColorAttachmentIndex(color_attachment);
    attachment_blend_state[color_attachment].dstColorBlendFactor = (VkBlendFactor)blend_factor;
  }

  void SetDstAlphaBlendFactor(size_t color_attachment, BlendFactor blend_factor) override {
    CheckColorAttachmentIndex(color_attachment);
    attachment_blend_state[color_attachment].dstAlphaBlendFactor = (VkBlendFactor)blend_factor;
  }

  void SetColorBlendOp(size_t color_attachment, BlendOp blend_op) override {
    CheckColorAttachmentIndex(color_attachment);
    attachment_blend_state[color_attachment].colorBlendOp = (VkBlendOp)blend_op;
  }

  void SetAlphaBlendOp(size_t color_attachment, BlendOp blend_op) override {
    CheckColorAttachmentIndex(color_attachment);
    attachment_blend_state[color_attachment].alphaBlendOp = (VkBlendOp)blend_op;
  }

  void SetColorWriteMask(size_t color_attachment, ColorComponent components) override {
    CheckColorAttachmentIndex(color_attachment);
    attachment_blend_state[color_attachment].colorWriteMask = (VkColorComponentFlags)components;
  }

  void SetBlendConstants(float r, float g, float b, float a) override {
    color_blend_info.blendConstants[0] = r;
    color_blend_info.blendConstants[1] = g;
    color_blend_info.blendConstants[2] = b;
    color_blend_info.blendConstants[3] = a;
  }

  auto Build() -> std::unique_ptr<GraphicsPipeline> override {
    auto number_of_color_attachments = own.render_pass->GetNumberOfColorAttachments();

    Assert(number_of_color_attachments <= kColorAttachmentLimit,
      "VulkanGraphicsPipelineBuilder: render pass with more than 32 color attachments is unsupported.");

    color_blend_info.attachmentCount = (u32)number_of_color_attachments;

    vertex_input_info.pVertexBindingDescriptions = vertex_input_bindings.data();
    vertex_input_info.vertexBindingDescriptionCount = (u32)vertex_input_bindings.size();
    vertex_input_info.pVertexAttributeDescriptions = vertex_input_attributes.data();
    vertex_input_info.vertexAttributeDescriptionCount = (u32)vertex_input_attributes.size();

    auto pipeline = VkPipeline{};

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &pipeline) != VK_SUCCESS) {
      Assert(false, "VulkanGraphicsPipelineBuilder: failed to create graphics pipeline");
    }

    return std::make_unique<VulkanGraphicsPipeline>(device, pipeline, own);
  }

private:
  static constexpr size_t kColorAttachmentLimit = 32;

  void SetScissorInternal(int x, int y, int width, int height) {
    scissor.offset.x = x;
    scissor.offset.y = y;
    scissor.extent.width = width;
    scissor.extent.height = height;
  }

  void CheckColorAttachmentIndex(size_t color_attachment) {
    Assert(color_attachment < kColorAttachmentLimit,
      "VulkanGraphicsPipelineBuilder: color_attachment was out-of-range");
  }

  auto GetVertexInputAttributeFormat(VertexDataType data_type, int components, bool normalized) -> VkFormat {
    // TODO: optimise this with a lookup table
    switch (data_type) {
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

    Assert(false, "VulkanGraphicsPipelineBuilder: failed to map vertex attribute format to VkFormat");
  }

  VulkanGraphicsPipeline::Own own;

  VkDevice device;

  bool have_explicit_scissor = false;

  VkViewport viewport{
    .x = 0.0f,
    .y = 0.0f,
    .width = 1.0f,
    .height = 1.0f,
    .minDepth = 0.0f,
    .maxDepth = 1.0f
  };

  VkRect2D scissor{
    .offset = {
      .x = 0,
      .y = 0
    },
    .extent = {
      .width = 1,
      .height = 1
    }
  };

  VkPipelineShaderStageCreateInfo pipeline_stages[2]{
    {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .stage = VK_SHADER_STAGE_VERTEX_BIT,
      .module = VK_NULL_HANDLE,
      .pName = "main",
      .pSpecializationInfo = nullptr
    },
    {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
      .module = VK_NULL_HANDLE,
      .pName = "main",
      .pSpecializationInfo = nullptr
    }
  };

  VkPipelineViewportStateCreateInfo viewport_info{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
    .pNext = nullptr,
    .flags = 0,
    .viewportCount = 1,
    .pViewports = &viewport,
    .scissorCount = 1,
    .pScissors = &scissor
  };

  VkPipelineRasterizationStateCreateInfo rasterization_info{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
    .pNext = nullptr,
    .flags = 0,
    .depthClampEnable = VK_FALSE,
    .rasterizerDiscardEnable = VK_FALSE,
    .polygonMode = VK_POLYGON_MODE_FILL,
    .cullMode = VK_CULL_MODE_NONE,
    .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
    .depthBiasEnable = VK_FALSE,
    .depthBiasConstantFactor = 0,
    .depthBiasClamp = 0,
    .depthBiasSlopeFactor = 0,
    .lineWidth = 1
  };

  VkPipelineMultisampleStateCreateInfo multisample_info{
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

  VkPipelineDepthStencilStateCreateInfo depth_stencil_info{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
    .pNext = nullptr,
    .flags = 0,
    .depthTestEnable = VK_FALSE,
    .depthWriteEnable = VK_FALSE,
    .depthCompareOp = VK_COMPARE_OP_ALWAYS,
    .depthBoundsTestEnable = VK_FALSE,
    .stencilTestEnable = VK_FALSE,
    .front = {},
    .back = {},
    .minDepthBounds = 0,
    .maxDepthBounds = 0
  };

  VkPipelineVertexInputStateCreateInfo vertex_input_info{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
    .pNext = nullptr,
    .flags = 0,
    .vertexBindingDescriptionCount = 0,
    .pVertexBindingDescriptions = nullptr,
    .vertexAttributeDescriptionCount = 0,
    .pVertexAttributeDescriptions = nullptr
  };

  VkPipelineInputAssemblyStateCreateInfo input_assembly_info{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
    .pNext = nullptr,
    .flags = 0,
    .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
    .primitiveRestartEnable = VK_FALSE
  };

  VkPipelineColorBlendAttachmentState attachment_blend_state[kColorAttachmentLimit]{
    {
      .blendEnable = VK_FALSE,
      .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
      .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
      .colorBlendOp = VK_BLEND_OP_ADD,
      .srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
      .dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
      .alphaBlendOp = VK_BLEND_OP_ADD,
      .colorWriteMask = 0xF
    }
  };

  VkPipelineColorBlendStateCreateInfo color_blend_info{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
    .pNext = nullptr,
    .flags = 0,
    .logicOpEnable = VK_FALSE,
    .logicOp = VK_LOGIC_OP_NO_OP,
    .attachmentCount = 1,
    .pAttachments = attachment_blend_state,
    .blendConstants = {0, 0, 0, 0}
  };

  std::vector<VkVertexInputBindingDescription> vertex_input_bindings;
  std::vector<VkVertexInputAttributeDescription> vertex_input_attributes;

  VkGraphicsPipelineCreateInfo pipeline_info{
    .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
    .pNext = nullptr,
    .flags = 0,
    .stageCount = 2,
    .pStages = pipeline_stages,
    .pVertexInputState = &vertex_input_info,
    .pInputAssemblyState = &input_assembly_info,
    .pTessellationState = nullptr,
    .pViewportState = &viewport_info,
    .pRasterizationState = &rasterization_info,
    .pMultisampleState = &multisample_info,
    .pDepthStencilState = &depth_stencil_info,
    .pColorBlendState = &color_blend_info,
    .pDynamicState = nullptr,
    .layout = VK_NULL_HANDLE,
    .renderPass = VK_NULL_HANDLE,
    .subpass = 0,
    .basePipelineHandle = VK_NULL_HANDLE,
    .basePipelineIndex = 0
  };
};

} // namespace Aura
