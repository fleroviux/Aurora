
// Copyright (C) 2022 fleroviux. All rights reserved.

#pragma once

#include <aurora/gal/backend/vulkan.hpp>
#include <aurora/log.hpp>
#include <vk_mem_alloc.h>

#include "texture_view.hpp"

namespace Aura {

struct VulkanTexture final : Texture {
 ~VulkanTexture() override {
    if (image_owned) {
      vmaDestroyImage(allocator, image, allocation);
    }
  }

  auto Handle() -> void* override {
    return (void*)image;
  }

  auto GetGrade() const -> Grade override {
    return grade;
  }

  auto GetFormat() const -> Format override {
    return format;
  }

  auto GetUsage() const -> Usage override {
    return usage;
  }

  auto GetWidth() const -> u32 override {
    return width;
  }

  auto GetHeight() const -> u32 override {
    return height;
  }

  auto GetDepth() const -> u32 override {
    return depth;
  }

  auto GetLayerCount() const -> u32 override {
    return range.layer_count;
  }

  auto GetMipCount() const -> u32 override {
    return range.mip_count;
  }

  auto DefaultSubresourceRange() const -> SubresourceRange override {
    return range;
  }

  auto DefaultView() const -> View const* override {
    return default_view.get();
  }

  auto DefaultView() -> View* override {
    return default_view.get();
  }

  auto CreateView(
    View::Type type,
    Format format,
    SubresourceRange const& range,
    ComponentMapping const& mapping = {}
  ) -> std::unique_ptr<View> override {
    return std::make_unique<VulkanTextureView>(device, this, type, format, range, mapping);
  }

  static auto Create2D(
    VkPhysicalDevice physical_device,
    VkDevice device,
    VmaAllocator allocator,
    u32 width,
    u32 height,
    u32 mip_levels,
    Format format,
    Usage usage
  ) -> std::unique_ptr<VulkanTexture> {
    auto image_info = VkImageCreateInfo{
      .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .imageType = VK_IMAGE_TYPE_2D,
      .format = (VkFormat)format,
      .extent = VkExtent3D{
        .width = width,
        .height = height,
        .depth = 1
      },
      .mipLevels = mip_levels,
      .arrayLayers = 1,
      .samples = VK_SAMPLE_COUNT_1_BIT,
      .tiling = VK_IMAGE_TILING_OPTIMAL,
      .usage = (VkImageUsageFlags)usage,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .queueFamilyIndexCount = 0,
      .pQueueFamilyIndices = nullptr,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
    };

    auto alloc_info = VmaAllocationCreateInfo{
      .usage = VMA_MEMORY_USAGE_AUTO
    };

    auto image = VkImage{};
    auto allocation = VmaAllocation{};

    if (vmaCreateImage(allocator, &image_info, &alloc_info, &image, &allocation, nullptr) != VK_SUCCESS) {
      Assert(false, "VulkanTexture: failed to create image");
    }

    auto texture = std::make_unique<VulkanTexture>();

    texture->device = device;
    texture->allocator = allocator;
    texture->allocation = allocation;
    texture->image = image;
    texture->grade = Grade::_2D;
    texture->format = format;
    texture->usage = usage;
    texture->width = width;
    texture->height = height;
    texture->depth = 1;
    texture->range = { (Aspect)GetImageAspectFromFormat(format), 0, mip_levels, 0, 1 };
    texture->image_owned = true;
    texture->default_view = texture->CreateView(
      View::Type::_2D, format, texture->DefaultSubresourceRange());

    return texture;
  }

  static auto Create2DFromSwapchain(
    VkDevice device,
    uint width,
    uint height,
    Format format,
    VkImage image
  ) -> std::unique_ptr<VulkanTexture> {
    auto texture = std::make_unique<VulkanTexture>();

    texture->device = device;
    texture->image = image;
    texture->grade = Grade::_2D;
    texture->format = format;
    texture->usage = Usage::ColorAttachment;
    texture->width = width;
    texture->height = height;
    texture->depth = 1;
    texture->range = { (Aspect)GetImageAspectFromFormat(format), 0, 1, 0, 1 };
    texture->image_owned = false;
    texture->default_view = texture->CreateView(
      View::Type::_2D, format, texture->DefaultSubresourceRange());

    return texture;
  }

  static auto CreateCube(
    VkPhysicalDevice physical_device,
    VkDevice device,
    VmaAllocator allocator,
    u32 width,
    u32 height,
    u32 mip_levels,
    Format format,
    Usage usage
  ) -> std::unique_ptr<VulkanTexture> {
    auto image_info = VkImageCreateInfo{
      .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
      .pNext = nullptr,
      .flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT,
      .imageType = VK_IMAGE_TYPE_2D,
      .format = (VkFormat)format,
      .extent = VkExtent3D{
        .width = width,
        .height = height,
        .depth = 1
      },
      .mipLevels = mip_levels,
      .arrayLayers = 6,
      .samples = VK_SAMPLE_COUNT_1_BIT,
      .tiling = VK_IMAGE_TILING_OPTIMAL,
      .usage = (VkImageUsageFlags)usage,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .queueFamilyIndexCount = 0,
      .pQueueFamilyIndices = nullptr,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
    };

    auto alloc_info = VmaAllocationCreateInfo{
      .usage = VMA_MEMORY_USAGE_AUTO
    };

    auto image = VkImage{};
    auto allocation = VmaAllocation{};

    if (vmaCreateImage(allocator, &image_info, &alloc_info, &image, &allocation, nullptr) != VK_SUCCESS) {
      Assert(false, "VulkanTexture: failed to create image");
    }

    auto texture = std::make_unique<VulkanTexture>();

    texture->device = device;
    texture->allocator = allocator;
    texture->allocation = allocation;
    texture->image = image;
    texture->grade = Grade::_2D;
    texture->format = format;
    texture->usage = usage;
    texture->width = width;
    texture->height = height;
    texture->depth = 1;
    texture->range = { (Aspect)GetImageAspectFromFormat(format), 0, mip_levels, 0, 6 };
    texture->image_owned = true;
    texture->default_view = texture->CreateView(
      View::Type::Cube, format, texture->DefaultSubresourceRange());

    return texture;
  }

private:
  static auto GetImageAspectFromFormat(Format format) -> VkImageAspectFlags {
    switch (format) {
      case Format::R8G8B8A8_SRGB:
      case Format::B8G8R8A8_SRGB:
      case Format::R32G32B32A32_SFLOAT:
        return VK_IMAGE_ASPECT_COLOR_BIT;
      case Format::DEPTH_F32:
        return VK_IMAGE_ASPECT_DEPTH_BIT;
      default:
        Assert(false, "VulkanTexture: cannot get image aspect for format: 0x{:08X}", (u32)format);
    }
  }

  VkDevice device;
  VmaAllocator allocator;
  VmaAllocation allocation;
  VkImage image;
  Grade grade;
  Format format;
  Usage usage;
  u32 width;
  u32 height;
  u32 depth;
  SubresourceRange range;
  std::unique_ptr<View> default_view;
  bool image_owned;
};

} // namespace Aura
