/*
 * Copyright (C) 2022 fleroviux
 */

#pragma once

#include <aurora/math/box3.hpp>
#include <aurora/math/plane.hpp>

namespace Aura {

/**
 * A frustum defined by six inward-facing parametric planes,
 * one {@link #Plane} for each {@link #Side} of the Frustum.
 */
struct Frustum {
  /**
   * Enumerate the six sides (or faces) of a Frustum.
   */
  enum class Side {
    NX = 0, /**< negative-X */
    PX = 1, /**< positive-X */
    NY = 2, /**< negative-Y */
    PY = 3, /**< positive-Y */
    NZ = 4, /**< negative-Z */
    PZ = 5  /**< positive-Z */
  };

  /**
   * Get the parametric {@link #Plane} for one {@link #Side} of this Frustum.
   *
   * @param side the side (or face)
   * @return the parametric {@link #Plane}
   */
  auto get_plane(Side side) const -> Plane const& {
    return planes[(int)side];
  }

  /**
   * Set the parametric {@link #Plane} for one {@link #Side} of this Frustum.
   *
   * @param side the side (or face)
   * @param the parametric {@link #Plane}
   */
  void set_plane(Side side, Plane const& plane) {
    planes[(int)side] = plane;
  }

  /**
   * Calculate whether an axis-aligned bounding box ({@link #Box3}) is
   * at least partially contained within this Frustum.
   *
   * @param box the bounding box
   * @return true if the {@link #Box3} is partially or fully inside this Frustum
   */
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
