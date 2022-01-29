/*
 * Copyright (C) 2022 fleroviux
 */

#pragma once

#include <aurora/gal/backend/vulkan.hpp>
#include <aurora/log.hpp>

namespace Aura {

struct VulkanTexture final : GPUTexture {
 ~VulkanTexture() override {
    vkDestroyImageView(device_, image_view_, nullptr);
    if (image_owned_) {
      vkDestroyImage(device_, image_, nullptr);
      vkFreeMemory(device_, memory_, nullptr);
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

  static auto create(
    VkPhysicalDevice physical_device,
    VkDevice device,
    u32 width,
    u32 height,
    Format format,
    Usage usage
  ) -> std::unique_ptr<VulkanTexture> {
    auto info = VkImageCreateInfo{
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
      .mipLevels = 1,
      .arrayLayers = 1,
      .samples = VK_SAMPLE_COUNT_1_BIT,
      .tiling = VK_IMAGE_TILING_OPTIMAL,
      .usage = (VkImageUsageFlags)usage,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .queueFamilyIndexCount = 0,
      .pQueueFamilyIndices = nullptr,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
    };

    auto image = VkImage{};

    if (vkCreateImage(device, &info, nullptr, &image) != VK_SUCCESS) {
      Assert(false, "VulkanTexture: failed to create image");
    }

    auto texture = std::make_unique<VulkanTexture>();

    texture->device_ = device;
    texture->image_ = image;
    texture->grade_ = Grade::_2D;
    texture->format_ = format;
    texture->usage_ = usage;
    texture->width_ = width;
    texture->height_ = height;
    texture->depth_ = 1;
    texture->AllocateMemory(physical_device);
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
    texture->image_owned_ = false;
    texture->CreateImageView();

    return texture;
  }

private:
  void AllocateMemory(VkPhysicalDevice physical_device) {
    auto requirements = VkMemoryRequirements{};
    vkGetImageMemoryRequirements(device_, image_, &requirements);

    auto info = VkMemoryAllocateInfo{
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .pNext = nullptr,
      .allocationSize = requirements.size,
      .memoryTypeIndex = vk_find_memory_type(
        physical_device,
        requirements.memoryTypeBits,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
      )
    };

    if (vkAllocateMemory(device_, &info, nullptr, &memory_) != VK_SUCCESS) {
      Assert(false, "VulkanTexture: failed to allocate image memory, size={}", requirements.size);
    }

    vkBindImageMemory(device_, image_, memory_, 0);
  }

  void CreateImageView() {
    // TODO: select right aspect mask depending on texture format?
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
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1
      }
    };

    if (vkCreateImageView(device_, &info, nullptr, &image_view_) != VK_SUCCESS) {
      Assert(false, "VulkanTexture: failed to create image view");
    }
  }

  static auto GetImageAspectFromFormat(Format format) -> VkImageAspectFlags {
    switch (format) {
      case Format::B8G8R8B8_SRGB:
        return VK_IMAGE_ASPECT_COLOR_BIT;
      case Format::DEPTH_F32:
        return VK_IMAGE_ASPECT_DEPTH_BIT;
      default:
        Assert(false, "VulkanTexture: cannot get image aspect for format: 0x{:08X}", (u32)format);
    }
  }

  VkDevice device_;
  VkDeviceMemory memory_;
  VkImage image_;
  VkImageView image_view_;
  Grade grade_;
  Format format_;
  Usage usage_;
  u32 width_;
  u32 height_;
  u32 depth_;
  bool image_owned_ = true;
};

} // namespace Aura
