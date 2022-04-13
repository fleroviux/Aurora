/*
 * Copyright (C) 2022 fleroviux
 */

#pragma once

#include <aurora/math/vector.hpp>

namespace Aura {

namespace detail {

template<typename T, typename Vector3>
struct Plane {
  Plane() {}

  Plane(Vector3 const& normal) : normal(normal), distance(NumericConstants<T>::zero()) {
  }

  Plane(Vector3 const& normal, T distance) : normal(normal), distance(distance) {
  }

  auto x() -> T& { return normal.x(); }
  auto y() -> T& { return normal.y(); }
  auto z() -> T& { return normal.z(); }
  auto w() -> T& { return normal.w(); }

  auto x() const -> T { return normal.x(); }
  auto y() const -> T { return normal.y(); }
  auto z() const -> T { return normal.z(); }
  auto w() const -> T { return normal.w(); }

  auto get_normal() const -> Vector3 {
    return normal;
  }

  void set_normal(Vector3 const& normal) {
    this->normal = normal;
  }

  auto get_distance() const -> T {
    return distance;
  }

  void set_distance(T distance) {
    this->distance = distance;
  }

  auto get_distance_to_point(Vector3 const& point) const -> T {
    return point.x() * normal.x() +
           point.y() * normal.y() +
           point.z() * normal.z() - distance;
  }

private:
  Vector3 normal;
  T distance;
};

} // namespace Aura::detail

using Plane = detail::Plane<float, Vector3>;

} // namespace Aura
