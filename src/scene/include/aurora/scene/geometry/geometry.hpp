/*
 * Copyright (C) 2021 fleroviux
 */

#pragma once

#include <aurora/scene/geometry/index_buffer.hpp>
#include <aurora/scene/geometry/vertex_buffer.hpp>
#include <optional>

namespace Aura {

struct Geometry {
  IndexBuffer index_buffer;
  std::vector<VertexBuffer> buffers;
};

} // namespace Aura
