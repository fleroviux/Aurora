
// Copyright (C) 2022 fleroviux. All rights reserved.

#pragma once

#include <aurora/gal/backend/vulkan.hpp>
#include <aurora/log.hpp>
#include <vk_mem_alloc.h>

namespace Aura {

struct VulkanTexture final : GPUTexture {
 ~VulkanTexture() override {
    vkDestroyImageView(device_, image_view_, nullptr);
    if (image_owned_) {
      vmaDestroyImage(allocator, image_, allocation);
    }
  }

  auto handle() -> void* override { return (void*)image_view_; }
  auto handle2() -> void* override { return (void*)image_; }
  auto grade() const -> Grade override { return grade_; }
  auto format() const -> Format override { return format_; };
  auto usage() const -> Usage override { return usage_; }
  auto width() const -> u32 override { return width_; }
  auto height() const -> u32 override { return height_; }
  auto depth() const -> u32 override { return depth_; }
  auto layers() const -> u32 override { return layers_; }
  auto mip_levels() const -> u32 override { return mip_levels_; }

  static auto create(
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

    texture->device_ = device;
    texture->allocator = allocator;
    texture->allocation = allocation;
    texture->image_ = image;
    texture->grade_ = Grade::_2D;
    texture->format_ = format;
    texture->usage_ = usage;
    texture->width_ = width;
    texture->height_ = height;
    texture->depth_ = 1;
    texture->layers_ = 1;
    texture->mip_levels_ = mip_levels;
    texture->CreateImageView();

    return texture;
  }

  static auto from_swapchain_image(
    VkDevice device,
    uint width,
    uint height,
    Format format,
    VkImage image
  ) -> std::unique_ptr<VulkanTexture> {
    auto texture = std::make_unique<VulkanTexture>();

    texture->device_ = device;
    texture->image_ = image;
    texture->grade_ = Grade::_2D;
    texture->format_ = format;
    texture->usage_ = Usage::ColorAttachment;
    texture->width_ = width;
    texture->height_ = height;
    texture->depth_ = 1;
    texture->layers_ = 1;
    texture->image_owned_ = false;
    texture->CreateImageView();

    return texture;
  }

  static auto create_cube(
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

    texture->device_ = device;
    texture->allocator = allocator;
    texture->allocation = allocation;
    texture->image_ = image;
    texture->grade_ = Grade::_2D;
    texture->format_ = format;
    texture->usage_ = usage;
    texture->width_ = width;
    texture->height_ = height;
    texture->depth_ = 1;
    texture->layers_ = 6;
    texture->mip_levels_ = mip_levels;
    texture->CreateImageViewCube();

    return texture;
  }

private:
  void CreateImageView() {
    auto info = VkImageViewCreateInfo{
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .image = image_,
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format = (VkFormat)format_,
      .components = VkComponentMapping{
        .r = VK_COMPONENT_SWIZZLE_R,
        .g = VK_COMPONENT_SWIZZLE_G,
        .b = VK_COMPONENT_SWIZZLE_B,
        .a = VK_COMPONENT_SWIZZLE_A,
      },
      .subresourceRange = VkImageSubresourceRange{
        .aspectMask = GetImageAspectFromFormat(format()),
        .baseMipLevel = 0,
        .levelCount = mip_levels(),
        .baseArrayLayer = 0,
        .layerCount = 1
      }
    };

    if (vkCreateImageView(device_, &info, nullptr, &image_view_) != VK_SUCCESS) {
      Assert(false, "VulkanTexture: failed to create image view");
    }
  }

  void CreateImageViewCube() {
    auto info = VkImageViewCreateInfo{
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .image = image_,
      .viewType = VK_IMAGE_VIEW_TYPE_CUBE,
      .format = (VkFormat)format_,
      .components = VkComponentMapping{
        .r = VK_COMPONENT_SWIZZLE_R,
        .g = VK_COMPONENT_SWIZZLE_G,
        .b = VK_COMPONENT_SWIZZLE_B,
        .a = VK_COMPONENT_SWIZZLE_A,
      },
      .subresourceRange = VkImageSubresourceRange{
        .aspectMask = GetImageAspectFromFormat(format()),
        .baseMipLevel = 0,
        .levelCount = mip_levels(),
        .baseArrayLayer = 0,
        .layerCount = 6
      }
    };

    if (vkCreateImageView(device_, &info, nullptr, &image_view_) != VK_SUCCESS) {
      Assert(false, "VulkanTexture: failed to create image view");
    }
  }

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

  VkDevice device_;
  VkDeviceMemory memory_;
  VmaAllocator allocator;
  VmaAllocation allocation;
  VkImage image_;
  VkImageView image_view_;
  Grade grade_;
  Format format_;
  Usage usage_;
  u32 width_;
  u32 height_;
  u32 depth_;
  u32 layers_;
  u32 mip_levels_ = 1;
  bool image_owned_ = true;
};

} // namespace Aura
