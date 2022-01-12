/*
 * Copyright (C) 2022 fleroviux
 */

#pragma once

#include <aurora/gal/render_device.hpp>
#include <vulkan/vulkan.h>

namespace Aura {

struct VulkanRenderDeviceOptions {
  VkInstance instance;
  VkPhysicalDevice physical_device;
  VkDevice device;
};

auto CreateVulkanRenderDevice(
  VulkanRenderDeviceOptions options
) -> std::unique_ptr<RenderDevice>;

} // namespace Aura
