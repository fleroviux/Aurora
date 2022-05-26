/*
 * Copyright (C) 2022 fleroviux
 */

#pragma once

#include <aurora/gal/bind_group.hpp>
#include <aurora/gal/buffer.hpp>
#include <aurora/gal/pipeline_builder.hpp>
#include <aurora/gal/pipeline_layout.hpp>
#include <aurora/gal/render_target.hpp>
#include <aurora/gal/render_pass.hpp>
#include <aurora/any_ptr.hpp>
#include <aurora/array_view.hpp>
#include <aurora/integer.hpp>

namespace Aura {

// subset of VkIndexType:
// https://vulkan.lunarg.com/doc/view/latest/windows/apispec.html#VkIndexType
enum class IndexDataType {
  UInt16 = 0,
  UInt32 = 1
};

struct CommandBuffer {
  enum class OneTimeSubmit {
    No = 0,
    Yes = 1
  };

  virtual ~CommandBuffer() = default;

  virtual auto Handle() -> void* = 0;

  virtual void Begin(OneTimeSubmit one_time_submit) = 0;
  virtual void End() = 0;

  virtual void BeginRenderPass(
    AnyPtr<RenderTarget> render_target,
    AnyPtr<RenderPass> render_pass
  ) = 0;
  virtual void EndRenderPass() = 0;

  virtual void BindGraphicsPipeline(AnyPtr<GraphicsPipeline> pipeline) = 0;

  virtual void BindGraphicsBindGroup(
    u32 set,
    AnyPtr<PipelineLayout> pipeline_layout,
    AnyPtr<BindGroup> bind_group
  ) = 0;

  // TODO: find an efficient solution that supports std::unique_ptr.
  virtual void BindVertexBuffers(
    ArrayView<std::shared_ptr<Buffer>> buffers,
    u32 first_binding = 0
  ) = 0;

  virtual void BindIndexBuffer(
    AnyPtr<Buffer> buffer,
    IndexDataType data_type,
    size_t offset = 0
  ) = 0;

  virtual void DrawIndexed(u32 index_count) = 0;
};

} // namespace Aura
