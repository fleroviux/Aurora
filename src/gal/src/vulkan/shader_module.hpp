/*
 * Copyright (C) 2022 fleroviux
 */

#pragma once

#include <aurora/gal/backend/vulkan.hpp>
#include <aurora/integer.hpp>
#include <aurora/log.hpp>

namespace Aura {

struct VulkanShaderModule final : ShaderModule {
  VulkanShaderModule(
    VkDevice device,
    u32 const* spirv,
    size_t size
  )   : device(device) {
    auto info = VkShaderModuleCreateInfo{
      .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .codeSize = size,
      .pCode = spirv
    };

    if (vkCreateShaderModule(device, &info, nullptr, &shader_module) != VK_SUCCESS) {
      Assert(false, "VulkanShaderModule: failed to create shader module");
    }
  }

 ~VulkanShaderModule() override {
    vkDestroyShaderModule(device, shader_module, nullptr);
  } 

  auto Handle() -> void* override {
    return (void*)shader_module;
  }

private:
  VkDevice device;
  VkShaderModule shader_module;
};

} // namespace Aura
