
// Copyright (C) 2022 fleroviux

#include <algorithm>
#include <vulkan/vulkan.h>

#include "texture_cache.hpp"

namespace Aura {

TextureCache::TextureCache(std::shared_ptr<RenderDevice> render_device)
    : render_device(render_device) {
}

auto TextureCache::Get(AnyPtr<Texture2D> texture) -> Entry const& {
  auto& entry = cache[texture.get()];

  if (!entry.texture) {
    CreateTexture(entry, texture);
    CreateSampler(entry);
    Upload(entry, texture);
  }

  return entry;
}

void TextureCache::SetCommandBuffer(CommandBuffer* command_buffer) {
  this->command_buffer = command_buffer;
}

void TextureCache::CreateTexture(Entry& entry, AnyPtr<Texture2D> texture) {
  auto width = texture->width();
  auto height = texture->height();
  auto usage = Texture::Usage::CopyDst | Texture::Usage::Sampled;
  auto mip_count = GetNumberOfMips(width, height);

  if (mip_count > 1) {
    usage = usage | Texture::Usage::CopySrc;
  }

  entry.texture = render_device->CreateTexture2D(
    width,
    height,
    Texture::Format::R8G8B8A8_SRGB,
    usage,
    mip_count
  );
}

void TextureCache::CreateSampler(Entry& entry) {
  entry.sampler = render_device->CreateSampler(Sampler::Config{
    .address_mode_u = Sampler::AddressMode::Repeat,
    .address_mode_v = Sampler::AddressMode::Repeat,
    .address_mode_w = Sampler::AddressMode::Repeat,
    .mag_filter = Sampler::FilterMode::Linear,
    .min_filter = Sampler::FilterMode::Linear,
    .mip_filter = Sampler::FilterMode::Linear,
    .anisotropy = true,
    .max_anisotropy = 16
  });
}

void TextureCache::Upload(Entry& entry, AnyPtr<Texture2D> texture) {
  auto width = texture->width();
  auto height = texture->height();
  auto buffer_size = width * height * sizeof(u32);

  if (!entry.staging_buffer || entry.staging_buffer->Size() != buffer_size) {
    entry.staging_buffer = render_device->CreateBufferWithData(
      Buffer::Usage::CopySrc, texture->data(), buffer_size);
  }

  auto barrier = MemoryBarrier{
    entry.texture,
    Access::None,
    Access::TransferWrite,
    Texture::Layout::Undefined,
    Texture::Layout::CopyDst
  };

  command_buffer->PipelineBarrier(PipelineStage::TopOfPipe, PipelineStage::Transfer, {&barrier, 1});

  // copy the image data from the staging buffer to the texture.
  // TODO: move the following code to the backend:
  {
    auto region = VkBufferImageCopy{
      .bufferOffset = 0,
      .bufferRowLength = width,
      .bufferImageHeight = height,
      .imageSubresource = VkImageSubresourceLayers{
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .mipLevel = 0,
        .baseArrayLayer = 0,
        .layerCount = 1
      },
      .imageOffset = VkOffset3D{
        .x = 0,
        .y = 0,
        .z = 0
      },
      .imageExtent = VkExtent3D{
        .width = width,
        .height = height,
        .depth = 1
      }
    };

    auto vk_cmd_buf = (VkCommandBuffer)command_buffer->Handle();
    auto vk_buffer = (VkBuffer)entry.staging_buffer->Handle();
    auto vk_image = (VkImage)entry.texture->Handle();

    vkCmdCopyBufferToImage(vk_cmd_buf, vk_buffer, vk_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
  }

  if (entry.texture->GetMipCount() > 1) {
    GenerateMipMaps(entry.texture);
  } else {
    // TODO: narrow down the pipeline stages that we block.
    barrier.src_access_mask = Access::TransferWrite;
    barrier.dst_access_mask = Access::ShaderRead;
    barrier.texture_info.src_layout = Texture::Layout::CopyDst;
    barrier.texture_info.dst_layout = Texture::Layout::ShaderReadOnly;
    barrier.texture_info.range = entry.texture->DefaultSubresourceRange();
    command_buffer->PipelineBarrier(PipelineStage::Transfer, PipelineStage::AllGraphics, {&barrier, 1});
  }
}

void TextureCache::GenerateMipMaps(AnyPtr<Texture> texture) {
  const int mip_count = texture->GetMipCount();

  auto mip_width = texture->GetWidth();
  auto mip_height = texture->GetHeight();

  auto barrier = MemoryBarrier{
    texture,
    Access::TransferWrite,
    Access::TransferRead,
    Texture::Layout::CopyDst,
    Texture::Layout::CopySrc,
    texture->DefaultSubresourceRange()
  };

  barrier.texture_info.range.mip_count = 1;

  command_buffer->PipelineBarrier(PipelineStage::Transfer, PipelineStage::Transfer, {&barrier, 1});

  for (int i = 1; i < mip_count; i++) {
    auto blit = VkImageBlit{
      .srcSubresource = VkImageSubresourceLayers{
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .mipLevel = (u32)(i - 1),
        .baseArrayLayer = 0,
        .layerCount = texture->GetLayerCount()
      },
      .dstSubresource = VkImageSubresourceLayers{
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .mipLevel = (u32)i,
        .baseArrayLayer = 0,
        .layerCount = texture->GetLayerCount()
      }
    };

    blit.srcOffsets[0] = {.x = 0, .y = 0, .z = 0};
    blit.srcOffsets[1] = {.x = (s32)mip_width, .y = (s32)mip_height, .z = 1};

    if (mip_width > 1) mip_width /= 2;
    if (mip_height > 1) mip_height /= 2;

    blit.dstOffsets[0] = {.x = 0, .y = 0, .z = 0};
    blit.dstOffsets[1] = {.x = (s32)mip_width, .y = (s32)mip_height, .z = 1};

    barrier.src_access_mask = Access::None;
    barrier.dst_access_mask = Access::TransferWrite;
    barrier.texture_info.src_layout = Texture::Layout::Undefined;
    barrier.texture_info.dst_layout = Texture::Layout::CopyDst;
    barrier.texture_info.range.base_mip = i;
    command_buffer->PipelineBarrier(PipelineStage::Transfer, PipelineStage::Transfer, {&barrier, 1});

    vkCmdBlitImage(
      (VkCommandBuffer)command_buffer->Handle(),
      (VkImage)texture->Handle(),
      VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
      (VkImage)texture->Handle(),
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      1, &blit,
      VK_FILTER_LINEAR
    );

    barrier.src_access_mask = Access::TransferWrite;
    barrier.dst_access_mask = Access::TransferRead;
    barrier.texture_info.src_layout = Texture::Layout::CopyDst;
    barrier.texture_info.dst_layout = Texture::Layout::CopySrc;
    command_buffer->PipelineBarrier(PipelineStage::Transfer, PipelineStage::Transfer, {&barrier, 1});
  }

  // TODO: narrow down the pipeline stages that we block.
  barrier.src_access_mask = Access::TransferRead;
  barrier.dst_access_mask = Access::ShaderRead;
  barrier.texture_info.src_layout = Texture::Layout::CopySrc;
  barrier.texture_info.dst_layout = Texture::Layout::ShaderReadOnly;
  barrier.texture_info.range.base_mip = 0;
  barrier.texture_info.range.mip_count = mip_count;
  command_buffer->PipelineBarrier(PipelineStage::Transfer, PipelineStage::AllGraphics, {&barrier, 1});
}

auto TextureCache::GetNumberOfMips(int width, int height, int depth) -> int {
  return (int)std::ceil(std::log2f((float)std::max(width, std::max(height, depth))));
}

} // namespace Aura
