/*
 * Copyright (C) 2022 fleroviux
 */

#pragma once

#include <aurora/gal/backend/vulkan.hpp>
#include <aurora/log.hpp>

namespace Aura {

struct VulkanTexture final : GPUTexture {
  VulkanTexture(
    VkDevice device,
    VkImage image,
    VkImageView image_view,
    Grade grade,
    Format format,
    Usage usage,
    u32 width,
    u32 height,
    u32 depth
  ) : device_(device)
    , image_(image)
    , image_view_(image_view)
    , grade_(grade)
    , format_(format)
    , usage_(usage)
    , width_(width)
    , height_(height)
    , depth_(depth)
    , image_owned_(false) {
  }

 ~VulkanTexture() override {
    vkDestroyImageView(device_, image_view_, nullptr);
    if (image_owned_) {
      vkDestroyImage(device_, image_, nullptr);
    }
  }

  static auto from_swapchain_image(
    VkDevice device,
    uint width,
    uint height,
    Format format,
    VkImage image
  ) -> std::unique_ptr<VulkanTexture> {
    auto info = VkImageViewCreateInfo{
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .image = image,
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format = (VkFormat)format,
      .components = VkComponentMapping{
        .r = VK_COMPONENT_SWIZZLE_R,
        .g = VK_COMPONENT_SWIZZLE_G,
        .b = VK_COMPONENT_SWIZZLE_B,
        .a = VK_COMPONENT_SWIZZLE_A,
      },
      .subresourceRange = VkImageSubresourceRange{
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1
      }
    };

    auto image_view = VkImageView{};

    if (vkCreateImageView(device, &info, nullptr, &image_view) != VK_SUCCESS) {
      Assert(false, "VulkanTexture: failed to create image view");
    }

    return std::make_unique<VulkanTexture>(
      device,
      image,
      image_view,
      Grade::_2D,
      Format::B8G8R8B8_SRGB,
      Usage::ColorAttachment | Usage::CopyDst,
      width,
      height,
      1
    );
  }

  auto handle() -> void* override { return (void*)image_view_; }
  auto grade() const -> Grade override { return grade_; }
  auto format() const -> Format override { return format_; };
  auto usage() const -> Usage override { return usage_; }
  auto width() const -> u32 override { return width_; }
  auto height() const -> u32 override { return height_; }
  auto depth() const -> u32 override { return depth_; }

private:
  VkDevice device_;
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