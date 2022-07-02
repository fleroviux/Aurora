
// Copyright (C) 2022 fleroviux. All rights reserved.

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
  auto Min() -> Vector3& { return min; }
  auto Max() -> Vector3& { return max; }

  auto Min() const -> Vector3 const& { return min; }
  auto Max() const -> Vector3 const& { return max; }

  /**
   * Apply a matrix transform on each vertex of this bounding box.
   * Because the new bounding box must be axis-aligned new mininum and maximum
   * vectors will be computed to fit the transformed bounding box.
   *
   * @param matrix the matrix transform
   * @return the transformed bounding box
   */
  auto ApplyMatrix(Matrix4 const& matrix) const -> Box3 {
    Box3 box;

    auto min_x = matrix.X().XYZ() * min.X();
    auto max_x = matrix.X().XYZ() * max.X();
    auto min_y = matrix.Y().XYZ() * min.Y();
    auto max_y = matrix.Y().XYZ() * max.Y();
    auto min_z = matrix.Z().XYZ() * min.Z();
    auto max_z = matrix.Z().XYZ() * max.Z();
    auto translation = matrix.W().XYZ();

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
      box.min.X() = std::min(box.min.X(), v[i].X());
      box.min.Y() = std::min(box.min.Y(), v[i].Y());
      box.min.Z() = std::min(box.min.Z(), v[i].Z());

      box.max.X() = std::max(box.max.X(), v[i].X());
      box.max.Y() = std::max(box.max.Y(), v[i].Y());
      box.max.Z() = std::max(box.max.Z(), v[i].Z());
    }

    return box;
  }

private:
  Vector3 min; /**< the lower-left vertex */
  Vector3 max; /**< the upper-right vertex */
};

} // namespace Aura
