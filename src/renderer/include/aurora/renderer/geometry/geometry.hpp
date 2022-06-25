/*
 * Copyright (C) 2022 fleroviux
 */

#pragma once

#include <aurora/math/box3.hpp>
#include <aurora/renderer/geometry/index_buffer.hpp>
#include <aurora/renderer/geometry/vertex_buffer.hpp>
#include <aurora/any_ptr.hpp>
#include <aurora/array_view.hpp>
#include <aurora/log.hpp>
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

  auto get_bounding_box() -> Box3 const& {
    if (!have_bounding_box) {
      compute_bounding_box();
    }

    return bounding_box;
  }

  void compute_bounding_box() {
    auto position_attribute = std::find_if(
      attributes.begin(), attributes.end(), [](Attribute const& attribute) {
        return attribute.location == 0;});

    if (position_attribute == attributes.end()) {
      return;
    }

    if (position_attribute->buffer >= vertex_buffers.size()) {
      return;
    }

    if (position_attribute->data_type != VertexDataType::Float32 &&
        position_attribute->components != 3) {
      Log<Warn>("Geometry: cannot calculate bounding box: position attribute is not in F32x3 format");
      return;
    }

    bounding_box.Min().X() = +std::numeric_limits<float>::infinity();
    bounding_box.Min().Y() = +std::numeric_limits<float>::infinity();
    bounding_box.Min().Z() = +std::numeric_limits<float>::infinity();

    bounding_box.Max().X() = -std::numeric_limits<float>::infinity();
    bounding_box.Max().Y() = -std::numeric_limits<float>::infinity();
    bounding_box.Max().Z() = -std::numeric_limits<float>::infinity();

    auto& buffer = vertex_buffers[position_attribute->buffer];
    auto  length = buffer->size() / buffer->stride();
    auto  offset = position_attribute->offset;

    for (size_t i = 0; i < length; i++) {
      auto position = buffer->read<Vector3>(i, offset, 0);

      bounding_box.Min().X() = std::min(bounding_box.Min().X(), position.X());
      bounding_box.Min().Y() = std::min(bounding_box.Min().Y(), position.Y());
      bounding_box.Min().Z() = std::min(bounding_box.Min().Z(), position.Z());

      bounding_box.Max().X() = std::max(bounding_box.Max().X(), position.X());
      bounding_box.Max().Y() = std::max(bounding_box.Max().Y(), position.Y());
      bounding_box.Max().Z() = std::max(bounding_box.Max().Z(), position.Z());
    }

    have_bounding_box = true;
  }

private:
  std::shared_ptr<IndexBuffer> index_buffer;
  std::vector<std::shared_ptr<VertexBuffer>> vertex_buffers;
  std::vector<Attribute> attributes;
  Topology topology = Topology::Triangles;
  Box3 bounding_box;
  bool have_bounding_box = false;
};

} // namespace Aura
