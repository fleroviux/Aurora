/*
 * Copyright (C) 2022 fleroviux
 */

#pragma once

#include <aurora/gal/backend/vulkan.hpp>

namespace Aura {

inline auto vk_find_memory_type(
  VkPhysicalDevice physical_device,
  u32 memory_type_bits,
  VkMemoryPropertyFlags property_flags
) -> u32 {
  auto memory_properties = VkPhysicalDeviceMemoryProperties{};

  // TODO: this should only be read out once during device creation.
  vkGetPhysicalDeviceMemoryProperties(physical_device, &memory_properties);

  for (u32 i = 0; i < memory_properties.memoryTypeCount; i++) {
    if (memory_type_bits & (1 << i)) {
      auto property_flags_have = memory_properties.memoryTypes[i].propertyFlags;
      if ((property_flags_have & property_flags) == property_flags) {
        return i;
      }
    }
  }

  Assert(false, "Vulkan: vk_find_memory_type() failed to find suitable memory type.");
}

} // namespace Aura
