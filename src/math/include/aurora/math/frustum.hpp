/*
 * Copyright (C) 2022 fleroviux
 */

#pragma once

#include <aurora/math/box3.hpp>
#include <aurora/math/plane.hpp>

namespace Aura {

struct Frustum {
  enum class Side {
    NX = 0,
    PX = 1,
    NY = 2,
    PY = 3,
    NZ = 4,
    PZ = 5
  };

  auto get_plane(Side side) const -> Plane {
    return planes[(int)side];
  }

  void set_plane(Side side, Plane plane) {
    planes[(int)side] = plane;
  }

  bool contains_box(Box3 const& box) const {
    for (auto& plane : planes) {
      auto point = Vector3{
        plane.x() > 0 ? box.max.x() : box.min.x(),
        plane.y() > 0 ? box.max.y() : box.min.y(),
        plane.z() > 0 ? box.max.z() : box.min.z()
      };

      if (plane.get_distance_to_point(point) < 0) {
        return false;
      }
    }

    return true;
  }

private:
  Plane planes[6];
};

} // namespace Aura
