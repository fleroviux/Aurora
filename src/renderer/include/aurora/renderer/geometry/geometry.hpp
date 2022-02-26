/*
 * Copyright (C) 2022 fleroviux
 */

#pragma once

#include <aurora/renderer/geometry/index_buffer.hpp>
#include <aurora/renderer/geometry/vertex_buffer.hpp>
#include <aurora/any_ptr.hpp>
#include <aurora/array_view.hpp>
#include <memory>
#include <optional>

namespace Aura {

struct Geometry final : GPUResource {
  struct Attribute {
    size_t location;
    size_t buffer;
    VertexDataType data_type;
    size_t components;
    bool normalized;
    size_t offset;
  };

  enum class Topology {
    Triangles
  };

  auto get_index_buffer() const -> std::shared_ptr<IndexBuffer> const& {
    return index_buffer;
  }

  void set_index_buffer(std::shared_ptr<IndexBuffer> index_buffer) {
    this->index_buffer = index_buffer;
    needs_update() = true;
  }

  auto get_vertex_buffers() const -> ArrayView<const std::shared_ptr<VertexBuffer>> {
    return ArrayView<const std::shared_ptr<VertexBuffer>>(
      (const std::shared_ptr<VertexBuffer>*)vertex_buffers.data(), vertex_buffers.size());
  }

  void add_vertex_buffer(std::shared_ptr<VertexBuffer> vertex_buffer) {
    vertex_buffers.push_back(vertex_buffer);
    needs_update() = true;
  }

  auto get_attributes() const -> ArrayView<const Attribute> {
    return ArrayView<const Attribute>((Attribute const*)attributes.data(), attributes.size());
  }

  void add_attribute(Attribute attribute) {
    attributes.push_back(attribute);
    needs_update() = true;
  }

  auto get_topology() const -> Topology {
    return topology;
  }

  void set_topology(Topology topology) {
    this->topology = topology;
  }

private:
  std::shared_ptr<IndexBuffer> index_buffer;
  std::vector<std::shared_ptr<VertexBuffer>> vertex_buffers;
  std::vector<Attribute> attributes;
  Topology topology = Topology::Triangles;
};

} // namespace Aura
