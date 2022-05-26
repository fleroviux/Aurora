/*
 * Copyright (C) 2022 fleroviux
 */

#pragma once

#include <aurora/gal/backend/vulkan.hpp>
#include <aurora/log.hpp>

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

  auto Build() -> std::unique_ptr<GraphicsPipeline> override {
    auto color_blend_attachment_info = VkPipelineColorBlendAttachmentState{
      .blendEnable = VK_FALSE,
      .colorWriteMask = 0xF
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

    pipeline_info.pColorBlendState = &color_blend_info;

    auto pipeline = VkPipeline{};

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &pipeline) != VK_SUCCESS) {
      Assert(false, "VulkanGraphicsPipelineBuilder: failed to create graphics pipeline");
    }

    return std::make_unique<VulkanGraphicsPipeline>(device, pipeline, own);
  }

private:
  void SetScissorInternal(int x, int y, int width, int height) {
    scissor.offset.x = x;
    scissor.offset.y = y;
    scissor.extent.width = width;
    scissor.extent.height = height;
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
    .pColorBlendState = nullptr,
    .pDynamicState = nullptr,
    .layout = VK_NULL_HANDLE,
    .renderPass = VK_NULL_HANDLE,
    .subpass = 0,
    .basePipelineHandle = VK_NULL_HANDLE,
    .basePipelineIndex = 0
  };
};

} // namespace Aura
