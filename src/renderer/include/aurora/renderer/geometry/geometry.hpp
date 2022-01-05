/*
 * Copyright (C) 2021 fleroviux
 */

#pragma once

#include <aurora/renderer/geometry/index_buffer.hpp>
#include <aurora/renderer/geometry/vertex_buffer.hpp>
#include <optional>

namespace Aura {

struct Geometry final : GPUResource {
  Geometry(IndexBuffer const& index_buffer) : index_buffer(index_buffer) {}

  Geometry(
    IndexBuffer const& index_buffer,
    std::vector<VertexBuffer> const& buffers
  ) : index_buffer(index_buffer), buffers(buffers) {}

  Geometry(IndexBuffer&& index_buffer) : index_buffer(std::move(index_buffer)) {}

  Geometry(
    IndexBuffer&& index_buffer,
    std::vector<VertexBuffer>&& buffers
  ) : index_buffer(std::move(index_buffer)), buffers(std::move(buffers)) {}

  enum class Topology {
    Triangles
  } topology = Topology::Triangles;

  IndexBuffer index_buffer;
  std::vector<VertexBuffer> buffers;
};

} // namespace Aura
