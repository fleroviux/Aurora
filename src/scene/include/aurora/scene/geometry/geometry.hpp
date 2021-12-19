/*
 * Copyright (C) 2021 fleroviux
 */

#pragma once

#include <aurora/scene/geometry/index_buffer.hpp>
#include <aurora/scene/geometry/vertex_buffer.hpp>

namespace Aura {

struct Geometry {
  IndexBuffer  indices;
  VertexBuffer vertices;
};

} // namespace Aura
