
// Copyright (C) 2022 fleroviux

#pragma once

#include <aurora/gal/render_device.hpp>
#include <aurora/renderer/texture.hpp>
#include <aurora/any_ptr.hpp>
#include <memory>
#include <unordered_map>

namespace Aura {

struct TextureCache {
  struct Entry {
    std::unique_ptr<Texture> texture;
    std::unique_ptr<Sampler> sampler;
    std::unique_ptr<Buffer> staging_buffer;
  };

  TextureCache(std::shared_ptr<RenderDevice> render_device);

  auto Get(AnyPtr<Texture2D> texture) -> Entry const&;
  void SetCommandBuffer(CommandBuffer* command_buffer);

private:
  void CreateTexture(Entry& entry, AnyPtr<Texture2D> texture);
  void CreateSampler(Entry& entry);
  void Upload(Entry& entry, AnyPtr<Texture2D> texture);
  void GenerateMipMaps(AnyPtr<Texture> texture);

  static auto GetNumberOfMips(int width, int height, int depth = 1) -> int;

  std::shared_ptr<RenderDevice> render_device;
  CommandBuffer* command_buffer;

  std::unordered_map<Texture2D*, Entry> cache;
};

} // namespace Aura
