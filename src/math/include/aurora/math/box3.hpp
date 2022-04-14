/*
 * Copyright (C) 2022 fleroviux
 */

#pragma once

#include <algorithm>
#include <aurora/math/matrix4.hpp>
#include <limits>

namespace Aura {

/**
 * A 3D axis-aligned bounding box (AABB).
 * The bounding box is defined through a minimum and a maximum vector.
 * Because the bounding box is axis-aligned, its eight vertices can be retrieved like this:
 *
 *   v0 = (min.x, min.y, min.z)
 *   v1 = (max.x, min.y, min.z)
 *   v2 = (min.x, max.y, min.z)
 *   v3 = (max.x, max.y, min.z)
 *   v4 = (min.x, min.y, max.z)
 *   v5 = (max.x, min.y, max.z)
 *   v6 = (min.x, max.y, max.z)
 *   v7 = (max.x, max.y, max.z)
 */
struct Box3 {
  Vector3 min; /**< the lower-left vertex */
  Vector3 max; /**< the upper-right vertex */

  /**
   * Apply a matrix transform on each vertex of this bounding box.
   * Because the new bounding box must be axis-aligned new mininum and maximum
   * vectors will be computed to fit the transformed bounding box.
   *
   * @param matrix the matrix transform
   * @return the transformed bounding box
   */
  auto apply_matrix(Matrix4 const& matrix) const -> Box3 {
    Box3 box;

    auto min_x = matrix.x().xyz() * min.x();
    auto max_x = matrix.x().xyz() * max.x();
    auto min_y = matrix.y().xyz() * min.y();
    auto max_y = matrix.y().xyz() * max.y();
    auto min_z = matrix.z().xyz() * min.z();
    auto max_z = matrix.z().xyz() * max.z();
    auto translation = matrix.w().xyz();

    Vector3 v[8];
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
