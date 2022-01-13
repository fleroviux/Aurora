/*
 * Copyright (C) 2022 fleroviux
 */

#include <aurora/gal/backend/vulkan.hpp>

#include "buffer.hpp"
#include "shader_module.hpp"

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
    BufferUsage usage, 
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
