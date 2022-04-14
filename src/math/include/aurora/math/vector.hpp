/*
 * Copyright (C) 2022 fleroviux
 */

#pragma once

#include <aurora/integer.hpp>
#include <aurora/math/numeric_traits.hpp>
#include <cmath>

namespace Aura {

namespace detail {

/**
 * Generic vector template on type `T` with dimension `n`.
 *
 * @tparam Derived the inheriting type
 * @tparam T       the underlying data type (i.e. float)
 * @tparam n       the number of components (or dimensions)
 */
template<typename Derived, typename T, uint n>
struct Vector {
  /**
   * Default constructor. The vector will be zero-initialised.
   */
  Vector() {}

  /**
   * Access a component of the vector via its index (between `0` and `n - 1`).
   * Out-of-bounds access is undefined behaviour.
   *
   * @param i the index
   * @return a reference to the component value
   */
  auto operator[](int i) -> T& {
    return data[i];
  }

  /**
   * Read a component of the vector via its index (between `0` and `n - 1`).
   * Out-of-bounds access is undefined behaviour.
   *
   * @param i the index
   * @return the component value
   */
  auto operator[](int i) const -> T {
    return data[i];
  }

  /**
   * Perform a component-wise summation of this vector with another vector.
   * Store the result in a new vector.
   *
   * @param other the other vector
   * @return the result vector
   */
  auto operator+(Derived const& other) const -> Derived {
    Derived result{};
    for (uint i = 0; i < n; i++)
      result[i] = data[i] + other[i];
    return result;
  }

  /**
   * Perform a component-wise subtraction of another vector from this vector.
   * Store the result in a new vector.
   *
   * @param other the other vector
   * @return the result vector
   */
  auto operator-(Derived const& other) const -> Derived {
    Derived result{};
    for (uint i = 0; i < n; i++)
      result[i] = data[i] - other[i];
    return result;
  }

  /**
   * Multiply each component of this vector with a scalar value.
   * Store the result in a new vector.
   *
   * @param value the scalar value
   * @return the result vector
   */
  auto operator*(T value) const -> Derived {
    Derived result{};
    for (uint i = 0; i < n; i++)
      result[i] = data[i] * value;
    return result;
  }

  /**
   * Divide each component of this vector by a scalar value.
   * Store the result in a new vector.
   *
   * @param value the scalar value
   * @return the result vector
   */
  auto operator/(T value) const -> Derived {
    Derived result{};
    for (uint i = 0; i < n; i++)
      result[i] = data[i] / value;
    return result;
  }

  /**
   * Perform a component-wise summation of this vector with another vector.
   * Store the result in this vector.
   *
   * @param other the other vector
   * @return a reference to this vector
   */
  auto operator+=(Derived const& other) -> Derived& {
    for (uint i = 0; i < n; i++)
      data[i] += other[i];
    return *static_cast<Derived*>(this);
  }

  /**
   * Perform a component-wise subtraction of another vector from this vector.
   * Store the result in this vector.
   *
   * @param other the other vector
   * @return a reference to this vector
   */
  auto operator-=(Derived const& other) -> Derived& {
    for (uint i = 0; i < n; i++)
      data[i] -= other[i];
    return *static_cast<Derived*>(this);
  }

  /**
   * Multiply each component of this vector with a scalar value.
   * Store the result in this vector.
   *
   * @param value the scalar value
   * @return a reference to this vector
   */
  auto operator*=(T value) -> Derived& {
    for (uint i = 0; i < n; i++)
      data[i] *= value;
    return *static_cast<Derived*>(this);
  }

  /**
   * Divide each component of this vector by a scalar value.
   * Store the result in this vector.
   *
   * @param value the scalar value
   * @return a reference to this vector
   */
  auto operator/=(T value) -> Derived& {
    for (uint i = 0; i < n; i++)
      data[i] /= value;
    return *static_cast<Derived*>(this);
  }

  /**
   * Negate each component in this vector. Store the result in a new vector.
   * @return the result vector
   */
  auto operator-() const -> Derived {
    Derived result{};
    for (uint i = 0; i < n; i++)
      result[i] = -data[i];
    return result;
  }

  /**
   * Perform a component-wise equality-comparison of this vector with another vector.
   * @return true if all components are equal.
   */
  bool operator==(Derived const& other) const {
    for (uint i = 0; i < n; i++)
      if (data[i] != other[i])
        return false;
    return true;
  }

  /**
   * Perform a component-wise inequality-comparison of this vector with another vector.
   * @return true if at least one component in unequal.
   */
  bool operator!=(Derived const& other) const {
    return !(*this == other);
  }

  /**
   * Calculate the dot product between this vector and another vector.
   * See: https://tutorial.math.lamar.edu/classes/calcii/dotproduct.aspx
   *
   * @param other the other vector
   * @return the scalar result
   */
  auto dot(Derived const& other) const -> T {
    T result{};
    for (uint i = 0; i < n; i++)
      result += data[i] * other[i];
    return result;
  }

  /**
   * Perform linear interpolation between two vectors with a factor between `0` and `1`.
   *
   * @param a the vector at `factor = 0`
   * @param b the vector at `factor = 1`
   * @return the interpolated result vector
   */
  static auto lerp(Derived const& a, Derived const& b, T factor) -> Derived {
    Derived result{};
    U one_minus_factor = NumericConstants<U>::one() - factor;
    for (uint i = 0; i < n; i++)
      result[i] = a[i] * one_minus_factor + b[i] * factor;
    return result;
  }

protected:
  T data[n] { NumericConstants<T>::zero() }; /**< the `n` components of the vector. */
};

/**
 * A two-dimensional vector template on type `T`
 *
 * @tparam Derived the inheriting type
 * @tparam T       the underlying data type (i.e. float)
 */
template<typename Derived, typename T>
struct Vector2 : Vector<Derived, T, 2> {
  using Vector<Derived, T, 2>::Vector;

  /**
   * Construct a Vector2 from two scalar values.
   */
  Vector2(T x, T y) {
    this->data[0] = x;
    this->data[1] = y;
  }

  auto x() -> T& { return this->data[0]; }
  auto y() -> T& { return this->data[1]; }

  auto x() const -> T { return this->data[0]; }
  auto y() const -> T { return this->data[1]; }
};

/**
 * A three-dimensional vector template on type `T`
 *
 * @tparam Derived the inheriting type
 * @tparam T       the underlying data type (i.e. float)
 */
template<typename Derived, typename T>
struct Vector3 : Vector<Derived, T, 3> {
  using Vector<Derived, T, 3>::Vector;

  /**
   * Construct a Vector3 from three scalar values.
   */
  Vector3(T x, T y, T z) {
    this->data[0] = x;
    this->data[1] = y;
    this->data[2] = z;
  }

  auto x() -> T& { return this->data[0]; }
  auto y() -> T& { return this->data[1]; }
  auto z() -> T& { return this->data[2]; }

  auto x() const -> T { return this->data[0]; }
  auto y() const -> T { return this->data[1]; }
  auto z() const -> T { return this->data[2]; }

  /**
   * Calculate the cross product of this vector with another vector.
   * See: https://tutorial.math.lamar.edu/classes/calcii/crossproduct.aspx
   *
   * @param other the other vector
   * @returm the result vector
   */
  auto cross(Vector3 const& other) -> Vector3 {
    return {
      this->data[1] * other[2] - this->data[2] * other[1],
      this->data[2] * other[0] - this->data[0] * other[2],
      this->data[0] * other[1] - this->data[1] * other[0]
    };
  }
};

/**
 * A four-dimensional vector template on type `T`
 *
 * @tparam Derived the inheriting type
 * @tparam Vector3 the Vector3 type used with this class.
 * @tparam T       the underlying data type (i.e. float)
 */
template<typename Derived, typename Vector3, typename T>
struct Vector4 : Vector<Derived, T, 4> {
  using Vector<Derived, T, 4>::Vector;

  /**
   * Construct a Vector4 from four scalar values.
   */
  Vector4(T x, T y, T z, T w) {
    this->data[0] = x;
    this->data[1] = y;
    this->data[2] = z;
    this->data[3] = w;
  }

  /**
   * Construct a Vector4 from a Vector3 and a scalar w-component.
   */
  Vector4(Vector3 const& xyz, T w = NumericConstants<T>::one()) {
    this->data[0] = xyz.x();
    this->data[1] = xyz.y();
    this->data[2] = xyz.z();
    this->data[3] = w;
  }

  auto x() -> T& { return this->data[0]; }
  auto y() -> T& { return this->data[1]; }
  auto z() -> T& { return this->data[2]; }
  auto w() -> T& { return this->data[3]; }

  auto x() const -> T { return this->data[0]; }
  auto y() const -> T { return this->data[1]; }
  auto z() const -> T { return this->data[2]; }
  auto w() const -> T { return this->data[3]; }

  auto xyz() const -> Vector3 { return Vector3{x(), y(), z()}; }
};

} // namespace Aura::detail

/**
 * A two-dimensional float vector 
 */
struct Vector2 final : detail::Vector2<Vector2, float> {
  using detail::Vector2<Vector2, float>::Vector2;
};

/**
 * A three-dimensional float vector
 */
struct Vector3 final : detail::Vector3<Vector3, float> {
  using detail::Vector3<Vector3, float>::Vector3;

  /**
   * Calculate the euclidean length of this vector.
   * @return the calculated length
   */
  auto length() const -> float {
    return std::sqrt(dot(*this));
  }

  /**
   * Set the euclidean length of this vector to one while preserving its direction.
   * If this vector is the zero vector then this operation is undefined.
   * @return a reference to this vector
   */
  auto normalize() -> Vector3& {
    *this *= 1.0f / length();
    return *this;
  }
};

/**
 * A four-dimensional float vector
 */
struct Vector4 final : detail::Vector4<Vector4, Vector3, float> {
  using detail::Vector4<Vector4, Vector3, float>::Vector4;
};

} // namespaace Aura
