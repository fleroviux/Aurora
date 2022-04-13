/*
 * Copyright (C) 2022 fleroviux
 */

#pragma once

#include <algorithm>
#include <aurora/math/matrix4.hpp>
#include <limits>

namespace Aura {

struct Box3 {
  Vector3 min;
  Vector3 max;

  auto apply_matrix(Matrix4 const& matrix) const -> Box3 {
    Box3 box;

    auto min_x = matrix.x().xyz() * min.x();
    auto max_x = matrix.x().xyz() * max.x();
    auto min_y = matrix.y().xyz() * min.y();
    auto max_y = matrix.y().xyz() * max.y();
    auto min_z = matrix.z().xyz() * min.z();
    auto max_z = matrix.z().xyz() * max.z();
    auto translation = matrix.w().xyz();

    // TODO: fix the matrix types (ugh)
    detail::Vector3<float> v[8];
    v[0] = min_x + min_y + min_z + translation;
    v[1] = max_x + min_y + min_z + translation;
    v[2] = min_x + max_y + min_z + translation;
    v[3] = max_x + max_y + min_z + translation;
    v[4] = min_x + min_y + max_z + translation;
    v[5] = max_x + min_y + max_z + translation;
    v[6] = min_x + max_y + max_z + translation;
    v[7] = max_x + max_y + max_z + translation;

    box.min = Vector3{
      +std::numeric_limits<float>::infinity(),
      +std::numeric_limits<float>::infinity(),
      +std::numeric_limits<float>::infinity()
    };

    box.max = Vector3{
      -std::numeric_limits<float>::infinity(),
      -std::numeric_limits<float>::infinity(),
      -std::numeric_limits<float>::infinity()
    };

    for (int i = 0; i < 8; i++) {
      box.min.x() = std::min(box.min.x(), v[i].x());
      box.min.y() = std::min(box.min.y(), v[i].y());
      box.min.z() = std::min(box.min.z(), v[i].z());

      box.max.x() = std::max(box.max.x(), v[i].x());
      box.max.y() = std::max(box.max.y(), v[i].y());
      box.max.z() = std::max(box.max.z(), v[i].z());
    }

    return box;
  }
};

} // namespace Aura
