
// Copyright (C) 2022 fleroviux. All rights reserved.

#include <aurora/gal/backend/vulkan.hpp>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include "bind_group.hpp"
#include "buffer.hpp"
#include "command_buffer.hpp"
#include "command_pool.hpp"
#include "fence.hpp"
#include "pipeline_builder.hpp"
#include "pipeline_layout.hpp"
#include "queue.hpp"
#include "render_target.hpp"
#include "sampler.hpp"
#include "shader_module.hpp"
#include "texture.hpp"

namespace Aura {

struct VulkanRenderDevice final : RenderDevice {
  VulkanRenderDevice(VulkanRenderDeviceOptions const& options)
      : instance(options.instance)
      , physical_device(options.physical_device)
      , device(options.device)
      , queue_family_graphics(options.queue_family_graphics) {
    CreateVmaAllocator();
    CreateDescriptorPool();
    CreateQueues();
  }

 ~VulkanRenderDevice() {
    vkDestroyDescriptorPool(device, descriptor_pool, nullptr);
    vmaDestroyAllocator(allocator);
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
      allocator,
      transfer_cmd_buffer,
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
    Texture::Format format,
    Texture::Usage usage,
    u32 mip_levels = 1
  ) -> std::unique_ptr<Texture> override {
    return VulkanTexture::create(physical_device, device, allocator, width, height, mip_levels, format, usage);
  }

  auto CreateTexture2DFromSwapchainImage(
    u32 width,
    u32 height,
    Texture::Format format,
    void* image_handle
  ) -> std::unique_ptr<Texture> override {
    return VulkanTexture::from_swapchain_image(device, width, height, format, (VkImage)image_handle);
  }

  auto CreateTextureCube(
    u32 width,
    u32 height,
    Texture::Format format,
    Texture::Usage usage,
    u32 mip_levels = 1
  ) -> std::unique_ptr<Texture> override {
    return VulkanTexture::create_cube(physical_device, device, allocator, width, height, mip_levels, format, usage);
  }

  auto CreateSampler(
    Sampler::Config const& config
  ) -> std::unique_ptr<Sampler> override {
    return std::make_unique<VulkanSampler>(device, config);
  }

  auto CreateRenderTarget(
    std::vector<std::shared_ptr<Texture>> const& color_attachments,
    std::shared_ptr<Texture> depth_stencil_attachment = {}
  ) -> std::unique_ptr<RenderTarget> override {
    return std::make_unique<VulkanRenderTarget>(device, color_attachments, depth_stencil_attachment);
  }

  auto CreateBindGroupLayout(
    std::vector<BindGroupLayout::Entry> const& entries
  ) -> std::shared_ptr<BindGroupLayout> override {
    return std::make_shared<VulkanBindGroupLayout>(device, descriptor_pool, entries);
  }

  auto CreatePipelineLayout(
    std::vector<std::shared_ptr<BindGroupLayout>> const& bind_groups
  ) -> std::unique_ptr<PipelineLayout> override {
    return std::make_unique<VulkanPipelineLayout>(device, bind_groups);
  }

  auto CreateGraphicsPipelineBuilder() -> std::unique_ptr<GraphicsPipelineBuilder> override {
    return std::make_unique<VulkanGraphicsPipelineBuilder>(device);
  }

  auto CreateGraphicsCommandPool(CommandPool::Usage usage) -> std::shared_ptr<CommandPool> override {
    return std::make_shared<VulkanCommandPool>(device, queue_family_graphics, usage);
  }

  auto CreateCommandBuffer(
    std::shared_ptr<CommandPool> pool
  ) -> std::unique_ptr<CommandBuffer> override {
    return std::make_unique<VulkanCommandBuffer>(device, pool);
  }

  auto CreateFence() -> std::unique_ptr<Fence> override {
    return std::make_unique<VulkanFence>(device);
  }

  auto GraphicsQueue() -> Queue* override {
    return graphics_queue.get();
  }

  void SetTransferCommandBuffer(CommandBuffer* cmd_buffer) override {
    transfer_cmd_buffer = (VulkanCommandBuffer*)cmd_buffer;
  }

private:
  void CreateVmaAllocator() {
    auto info = VmaAllocatorCreateInfo{};
    info.flags = 0;
    info.physicalDevice = physical_device;
    info.device = device;
    info.preferredLargeHeapBlockSize = 0;
    info.pAllocationCallbacks = nullptr;
    info.pDeviceMemoryCallbacks = nullptr;
    info.pHeapSizeLimit = nullptr;
    info.pVulkanFunctions = nullptr;
    info.instance = instance;
    info.vulkanApiVersion = VK_API_VERSION_1_2;
    info.pTypeExternalMemoryHandleTypes = nullptr;

    if (vmaCreateAllocator(&info, &allocator) != VK_SUCCESS) {
      Assert(false, "VulkanRenderDevice: failed to create the VMA allocator");
    }
  }

  void CreateDescriptorPool() {
    // TODO: create pools for other descriptor types
    VkDescriptorPoolSize pool_sizes[] {
      {
        .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 4096
      },
      {
        .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = 4096
      }
    };

    auto info = VkDescriptorPoolCreateInfo{
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      .pNext = nullptr,
      .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
      .maxSets = 4096,
      .poolSizeCount = (u32)(sizeof(pool_sizes) / sizeof(VkDescriptorPoolSize)),
      .pPoolSizes = pool_sizes
    };

    if (vkCreateDescriptorPool(device, &info, nullptr, &descriptor_pool) != VK_SUCCESS) {
      Assert(false, "VulkanRenderDevice: failed to create descriptor pool");
    }
  }

  void CreateQueues() {
    VkQueue graphics;
    vkGetDeviceQueue(device, queue_family_graphics, 0, &graphics);
    graphics_queue = std::make_unique<VulkanQueue>(graphics);
  }

  VkInstance instance;
  VkPhysicalDevice physical_device;
  VkDevice device;
  VkDescriptorPool descriptor_pool;
  VmaAllocator allocator;
  VulkanCommandBuffer* transfer_cmd_buffer;
  std::unique_ptr<VulkanQueue> graphics_queue;
  u32 queue_family_graphics;
};

auto CreateVulkanRenderDevice(
  VulkanRenderDeviceOptions const& options
) -> std::unique_ptr<RenderDevice> {
  return std::make_unique<VulkanRenderDevice>(options);
}

} // namespace Aura
