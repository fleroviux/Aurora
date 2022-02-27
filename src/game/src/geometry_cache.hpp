/*
 * Copyright (C) 2022 fleroviux
 */

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
    Buffer* ibo;
    std::vector<Buffer*> vbos;
  };

  GeometryCache(std::shared_ptr<RenderDevice> render_device)
    : render_device(render_device) {}

  auto Get(AnyPtr<Geometry> geometry) -> Entry const& {
    auto handle = geometry.get();
    auto& entry = geo_cache[handle];

    if (!entry.exist || geometry->needs_update()) {
      if (!entry.exist) {
        geometry->add_release_callback([this, handle]() {
          geo_cache.erase(handle);
        });
        entry.exist = true;
      } else {
        entry.vbos.clear();
      }

      entry.ibo = GetIBO(geometry->get_index_buffer());

      for (auto& vertex_buffer : geometry->get_vertex_buffers()) {
        entry.vbos.push_back(GetVBO(vertex_buffer));
      }

      geometry->needs_update() = false;
    }

    return entry;
  }

private:
  auto GetIBO(std::shared_ptr<IndexBuffer> const& index_buffer) -> Buffer* {
    auto handle = index_buffer.get();
    auto& ibo = ibo_cache[handle];

    if (!ibo || index_buffer->needs_update()) {
      if (ibo && ibo->Size() == index_buffer->size()) {
        ibo->Update(index_buffer->data(), index_buffer->size());
      } else {
        if (!ibo) {
          index_buffer->add_release_callback([this, handle]() {
            ibo_cache.erase(handle);
          });
        }

        ibo = render_device->CreateBufferWithData(
          Buffer::Usage::IndexBuffer, index_buffer->view<u8>());
      }
      
      index_buffer->needs_update() = false;
    }

    return ibo.get();
  }

  auto GetVBO(std::shared_ptr<VertexBuffer> const& vertex_buffer) -> Buffer* {
    auto handle = vertex_buffer.get();
    auto& vbo = vbo_cache[handle];

    if (!vbo || vertex_buffer->needs_update()) {
      if (vbo && vbo->Size() == vertex_buffer->size()) {
        vbo->Update(vertex_buffer->data(), vertex_buffer->size());
      } else {
        if (!vbo) {
          vertex_buffer->add_release_callback([this, handle]() {
            vbo_cache.erase(handle);
          });
        }

        vbo = render_device->CreateBufferWithData(
          Buffer::Usage::VertexBuffer, vertex_buffer->view<u8>());
      }

      vertex_buffer->needs_update() = false;
    }

    return vbo.get();
  }

  std::unordered_map<IndexBuffer*, std::unique_ptr<Buffer>> ibo_cache;
  std::unordered_map<VertexBuffer*, std::unique_ptr<Buffer>> vbo_cache;
  std::unordered_map<Geometry*, Entry> geo_cache;

  std::shared_ptr<RenderDevice> render_device;
};

} // namespace Aura
