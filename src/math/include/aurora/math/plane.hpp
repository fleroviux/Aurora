/*
 * Copyright (C) 2022 fleroviux
 */

#pragma once

#include <aurora/math/vector.hpp>

namespace Aura {

namespace detail {

/**
 * A parametric plane defined by a normal vector and a signed distance from the origin along the normal.
 *
 * @tparam T the underlying data type (i.e. float)
 * @tparam Vec3 the three-dimensional vector type to be used
 */
template<typename T, typename Vec3>
struct Plane {
  /**
   * Default constructor. The plane is initialized to lie at (0, 0, 0) and point upwards (on the y-Axis).
   */
  Plane() {}

  /**
   * Construct a Plane from a normal vector and a signed distance.
   *
   * @param normal the normal vector
   * @param distance the signed distance from the origin
   */
  Plane(Vec3 const& normal, T distance = NumericConstants<T>::zero()) : normal(normal), distance(distance) {
  }

  auto x() -> T& { return normal.x(); }
  auto y() -> T& { return normal.y(); }
  auto z() -> T& { return normal.z(); }
  auto w() -> T& { return normal.w(); }

  auto x() const -> T { return normal.x(); }
  auto y() const -> T { return normal.y(); }
  auto z() const -> T { return normal.z(); }
  auto w() const -> T { return normal.w(); }

  /**
   * Get the normal vector of this plane.
   * @return the normal vector
   */
  auto get_normal() const -> Vec3 {
    return normal;
  }

  /**
   * Set the normal vector for this plane.
   * 
   * @param normal the normal vector
   */
  void set_normal(Vec3 const& normal) {
    this->normal = normal;
  }

  /**
   * Get the signed distance of this plane from the origin.
   */
  auto get_distance() const -> T {
    return distance;
  }

  /**
   * Set the signed distance from the origin for this plane.
   */
  void set_distance(T distance) {
    this->distance = distance;
  }

  /**
   * Compute the signed distance of a point from this plane.
   *
   * @param point the point
   * @return the signed distance
   */
  auto get_distance_to_point(Vec3 const& point) const -> T {
    return point.x() * normal.x() +
           point.y() * normal.y() +
           point.z() * normal.z() - distance;
  }

private:
  Vec3 normal{
    NumericConstants<T>::zero(),
    NumericConstants<T>::one(),
    NumericConstants<T>::zero()
  };
  T distance = NumericConstants<T>::zero();
};

} // namespace Aura::detail

using Plane = detail::Plane<float, Vector3>;

} // namespace Aura
