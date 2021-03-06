
// Copyright (C) 2022 fleroviux. All rights reserved.

#pragma once

#include <aurora/gal/backend/vulkan.hpp>
#include <aurora/log.hpp>

namespace Aura {

struct VulkanSampler final : Sampler {
  VulkanSampler(VkDevice device, Config const& config) : device_(device) {
    // TODO: clamp anisotropy level to the hardware-supported level.
    auto info = VkSamplerCreateInfo{
     .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
     .pNext = nullptr,
     .flags = 0,
     .magFilter = (VkFilter)config.mag_filter,
     .minFilter = (VkFilter)config.min_filter,
     .mipmapMode = (VkSamplerMipmapMode)config.mip_filter,
     .addressModeU = (VkSamplerAddressMode)config.address_mode_u,
     .addressModeV = (VkSamplerAddressMode)config.address_mode_v,
     .addressModeW = (VkSamplerAddressMode)config.address_mode_w,
     .mipLodBias = 0,
     .anisotropyEnable = config.anisotropy ? VK_TRUE : VK_FALSE,
     .maxAnisotropy = (float)config.max_anisotropy,
     .compareEnable = VK_FALSE,
     .compareOp = VK_COMPARE_OP_NEVER,
     .minLod = 0,
     .maxLod = VK_LOD_CLAMP_NONE,
     .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
     .unnormalizedCoordinates = VK_FALSE
    };

    if (vkCreateSampler(device, &info, nullptr, &sampler_) != VK_SUCCESS) {
      Assert(false, "VulkanSampler: failed to create sampler");
    }
  }

 ~VulkanSampler() override {
    vkDestroySampler(device_, sampler_, nullptr);
  }

  auto Handle() -> void* override {
    return (void*)sampler_;
  }

private:
  VkDevice device_;
  VkSampler sampler_;
};

} // namespace Aura
