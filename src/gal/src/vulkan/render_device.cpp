/*
 * Copyright (C) 2022 fleroviux
 */

#include <aurora/gal/backend/vulkan.hpp>

#include "bind_group.hpp"
#include "buffer.hpp"
#include "pipeline_layout.hpp"
#include "render_target.hpp"
#include "shader_module.hpp"
#include "texture.hpp"

namespace Aura {

struct VulkanRenderDevice final : RenderDevice {
  VulkanRenderDevice(VulkanRenderDeviceOptions options)
      : instance(options.instance)
      , physical_device(options.physical_device)
      , device(options.device) {
  }

  auto Handle() -> void* override {
    return (void*)device;
  }

  auto CreateBuffer(
    Buffer::Usage usage, 
    size_t size,
    bool host_visible = true,
    bool map = true
  ) -> std::unique_ptr<Buffer> override {
    return std::make_unique<VulkanBuffer>(
      physical_device,
      device,
      usage,
      size,
      host_visible,
      map
    );
  }

  auto CreateShaderModule(
    u32 const* spirv,
    size_t size
  ) -> std::unique_ptr<ShaderModule> override {
    return std::make_unique<VulkanShaderModule>(device, spirv, size);
  }

  auto CreateTexture2D(
    u32 width,
    u32 height,
    GPUTexture::Format format,
    GPUTexture::Usage usage
  ) -> std::unique_ptr<GPUTexture> override {
    return VulkanTexture::create(physical_device, device, width, height, format, usage);
  }

  auto CreateTexture2DFromSwapchainImage(
    u32 width,
    u32 height,
    GPUTexture::Format format,
    void* image_handle
  ) -> std::unique_ptr<GPUTexture> override {
    return VulkanTexture::from_swapchain_image(device, width, height, format, (VkImage)image_handle);
  }

  auto CreateRenderTarget(
    std::vector<GPUTexture*> const& color_attachments,
    GPUTexture* depth_stencil_attachment = nullptr
  ) -> std::unique_ptr<RenderTarget> override {
    return std::make_unique<VulkanRenderTarget>(device, color_attachments, depth_stencil_attachment);
  }

  auto CreateBindGroupLayout(
    std::vector<BindGroupLayout::Entry> const& entries
  ) -> std::shared_ptr<BindGroupLayout> override {
    return std::make_shared<VulkanBindGroupLayout>(device, entries);
  }

  auto CreatePipelineLayout(
    std::vector<std::shared_ptr<BindGroupLayout>> const& bind_groups
  ) -> std::unique_ptr<PipelineLayout> override {
    return std::make_unique<VulkanPipelineLayout>(device, bind_groups);
  }

private:
  VkInstance instance;
  VkPhysicalDevice physical_device;
  VkDevice device;
};

auto CreateVulkanRenderDevice(
  VulkanRenderDeviceOptions options
) -> std::unique_ptr<RenderDevice> {
  return std::make_unique<VulkanRenderDevice>(options);
}

} // namespace Aura
