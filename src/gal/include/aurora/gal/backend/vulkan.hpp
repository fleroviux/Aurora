
// Copyright (C) 2022 fleroviux. All rights reserved.

#pragma once

#include <aurora/gal/render_device.hpp>
#include <vulkan/vulkan.h>

namespace Aura {

struct VulkanRenderDeviceOptions {
  VkInstance instance;
  VkPhysicalDevice physical_device;
  VkDevice device;
  u32 queue_family_graphics;
};

auto CreateVulkanRenderDevice(
  VulkanRenderDeviceOptions const& options
) -> std::unique_ptr<RenderDevice>;

} // namespace Aura
