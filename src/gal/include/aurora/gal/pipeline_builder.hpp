/*
 * Copyright (C) 2022 fleroviux
 */

#pragma once

#include <aurora/gal/pipeline_layout.hpp>
#include <aurora/gal/render_pass.hpp>
#include <aurora/gal/shader_module.hpp>
#include <aurora/integer.hpp>
#include <memory>

namespace Aura {

// subset of VkPipelineStageFlagBits:
// https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/VkPipelineStageFlagBits.html
enum class PipelineStage {
  Vertex   = 0x00000008,
  Fragment = 0x00000080
};

// subset of VkPolygonMode:
// https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/VkPolygonMode.html
enum class PolygonMode {
  Fill = 0,
  Line = 1,
  Point = 2
};

// subset of VkCullModeFlagBits:
// https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/VkCullModeFlagBits.html
enum class PolygonFace {
  None = 0,
  Front = 1,
  Back = 2
};

// equivalent to VkCompareOp:
// https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/VkCompareOp.html
enum class CompareOp {
  Never = 0,
  Less = 1,
  Equal = 2,
  LessOrEqual = 3,
  Greater = 4,
  NotEqual = 5,
  GreaterOrEqual = 6,
  Always = 7
};

// equivalent to VkPrimitiveTopology
// https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/VkPrimitiveTopology.html
enum class PrimitiveTopology {
  PointList = 0,
  LineList = 1,
  LineStrip = 2,
  TriangleList = 3,
  TriangleStrip = 4,
  TriangleFan = 5,
  LineListWithAdjacency = 6,
  LineStripWithAdjacency = 7,
  TriangleListWithAdjacency = 8,
  TriangleStripWithAdjacency = 9,
  PatchList = 10
};

enum class VertexDataType {
  SInt8,
  UInt8,
  SInt16,
  UInt16,
  UInt32,
  Float16,
  Float32
};

// equivalent to VkVertexInputRate
// https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/VkVertexInputRate.html
enum class VertexInputRate {
  Vertex = 0,
  Instance = 1
};

// equivalent to VkBlendFactor
// https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/VkBlendFactor.html
enum class BlendFactor {
  Zero = 0,
  One = 1,
  SrcColor = 2,
  OneMinusSrcColor = 3,
  DstColor = 4,
  OneMinusDstColor = 5,
  SrcAlpha = 6,
  OneMinusSrcAlpha = 7,
  DstAlpha = 8,
  OneMinusDstAlpha = 9,
  ConstantColor = 10,
  OneMinusConstantColor = 11,
  ConstantAlpha = 12,
  OneMinusConstantAlpha = 13,
  SrcAlphaSaturate = 14,
  Src1Color = 15,
  OneMinusSrc1Color = 16,
  Src1Alpha = 17,
  OneMinusSrc1Alpha = 18
};

// subset of VkBlendOp:
// https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/VkBlendOp.html
enum class BlendOp {
  Add = 0,
  Subtract = 1,
  ReverseSubtract = 2,
  Min = 3,
  Max = 4
};

// equivalent to VkColorComponentFlagBits:
// https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/VkColorComponentFlagBits.html
enum class ColorComponent {
  R = 1,
  G = 2,
  B = 4,
  A = 8,
  RGB = R | G | B,
  RGBA = R | G | B | A
};

constexpr auto operator|(ColorComponent lhs, ColorComponent rhs) -> ColorComponent {
  return static_cast<ColorComponent>(static_cast<int>(lhs) | static_cast<int>(rhs));
}

struct GraphicsPipeline {
  virtual ~GraphicsPipeline() = default;

  virtual auto Handle() -> void* = 0;
};

struct GraphicsPipelineBuilder {
  virtual ~GraphicsPipelineBuilder() = default;

  virtual void SetViewport(int x, int y, int width, int height) = 0;
  virtual void SetScissor(int x, int y, int width, int height) = 0;
  virtual void SetShaderModule(PipelineStage stage, std::shared_ptr<ShaderModule> shader_module) = 0;
  virtual void SetPipelineLayout(std::shared_ptr<PipelineLayout> layout) = 0;
  virtual void SetRenderPass(std::shared_ptr<RenderPass> render_pass) = 0;
  virtual void SetRasterizerDiscardEnable(bool enable) = 0;
  virtual void SetPolygonMode(PolygonMode mode) = 0;
  virtual void SetPolygonCull(PolygonFace face) = 0;
  virtual void SetLineWidth(float width) = 0;
  virtual void SetDepthTestEnable(bool enable) = 0;
  virtual void SetDepthWriteEnable(bool enable) = 0;
  virtual void SetDepthCompareOp(CompareOp compare_op) = 0;
  virtual void SetPrimitiveTopology(PrimitiveTopology topology) = 0;
  virtual void SetPrimitiveRestartEnable(bool enable) = 0;
  virtual void SetBlendEnable(size_t color_attachment, bool enable) = 0;
  virtual void SetSrcColorBlendFactor(size_t color_attachment, BlendFactor blend_factor) = 0;
  virtual void SetSrcAlphaBlendFactor(size_t color_attachment, BlendFactor blend_factor) = 0;
  virtual void SetDstColorBlendFactor(size_t color_attachment, BlendFactor blend_factor) = 0;
  virtual void SetDstAlphaBlendFactor(size_t color_attachment, BlendFactor blend_factor) = 0;
  virtual void SetColorBlendOp(size_t color_attachment, BlendOp blend_op) = 0;
  virtual void SetAlphaBlendOp(size_t color_attachment, BlendOp blend_op) = 0;
  virtual void SetColorWriteMask(size_t color_attachment, ColorComponent components) = 0;
  virtual void SetBlendConstants(float r, float g, float b, float a) = 0;

  virtual void ResetVertexInput() = 0;
  virtual void AddVertexInputBinding(u32 binding, u32 stride, VertexInputRate input_rate = VertexInputRate::Vertex) = 0;
  virtual void AddVertexInputAttribute(
    u32 location,
    u32 binding,
    u32 offset,
    VertexDataType data_type,
    int components,
    bool normalized
  ) = 0;

  virtual auto Build() -> std::unique_ptr<GraphicsPipeline> = 0;
};

} // namespace Aura
