
// Copyright (C) 2022 fleroviux. All rights reserved.

#pragma once

#include <aurora/gal/render_device.hpp>
#include <aurora/renderer/geometry/geometry.hpp>
#include <aurora/any_ptr.hpp>
#include <unordered_map>
#include <vector>

namespace Aura {

struct GeometryCache {
  struct Entry {
    bool exist = false;
    std::shared_ptr<Buffer> ibo;
    std::vector<std::shared_ptr<Buffer>> vbos;
  };

  GeometryCache(std::shared_ptr<RenderDevice> render_device)
    : render_device(render_device) {}

  auto Get(AnyPtr<Geometry> geometry) -> Entry const&;

private:
  auto GetIBO(std::shared_ptr<IndexBuffer> const& index_buffer) -> std::shared_ptr<Buffer>;
  auto GetVBO(std::shared_ptr<VertexBuffer> const& vertex_buffer) -> std::shared_ptr<Buffer>;

  std::unordered_map<IndexBuffer*, std::shared_ptr<Buffer>> ibo_cache;
  std::unordered_map<VertexBuffer*, std::shared_ptr<Buffer>> vbo_cache;
  std::unordered_map<Geometry*, Entry> geo_cache;

  std::shared_ptr<RenderDevice> render_device;
};

} // namespace Aura
