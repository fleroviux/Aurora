/*
 * Copyright (C) 2022 fleroviux
 */

#pragma once

#include <algorithm>
#include <aurora/math/numeric_traits.hpp>
#include <aurora/math/matrix4.hpp>
#include <cmath>
#include <initializer_list>

namespace Aura {

namespace detail {

/**
 * Generic quaternion template on type `T`.
 * This implementation uses the WXYZ convention where W is the scalar part and XYZ the vector part.
 *
 * @tparam Derived the inheriting type
 * @tparam Vector3 the Vector3 type used with this class
 * @tparam T       the underlying data type (i.e. float)
 */
template<typename Derived, typename Vector3, typename T>
struct Quaternion {
  /**
   * Default constructor. The Quaternion will be initialised to (1 0 0 0).
   */
  Quaternion() {}

  /**
   * Construct a Quaternion from four scalar values.
   */
  Quaternion(T w, T x, T y, T z) : data{w, x, y, z} {}

  /**
   * Access a component of the quaternion via its index (between `0` and `3`).
   * Out-of-bounds access is undefined behaviour.
   *
   * @param index the index
   * @return a reference to the component value
   */
  auto operator[](int index) -> T& {
    return data[index];
  }

  /**
   * Read a component of the quaternion via its index (between `0` and `3`).
   * Out-of-bounds access is undefined behaviour.
   *
   * @param index the index
   * @return the component value
   */
  auto operator[](int index) const -> T {
    return data[index];
  }

  auto w() -> T& { return data[0]; }
  auto x() -> T& { return data[1]; }
  auto y() -> T& { return data[2]; }
  auto z() -> T& { return data[3]; }

  auto w() const -> T { return data[0]; }
  auto x() const -> T { return data[1]; }
  auto y() const -> T { return data[2]; }
  auto z() const -> T { return data[3]; }

  auto xyz() const -> Vector3 { return Vector3{x(), y(), z()}; }

  /**
   * Perform a component-wise summation of this quaternion with another quaternion.
   * Store the result in a new quaternion.
   *
   * @param rhs the other quaternion
   * @return the result quaternion
   */
  auto operator+(Derived const& rhs) const -> Derived {
    return Derived{
      w() + rhs.w(),
      x() + rhs.x(),
      y() + rhs.y(),
      z() + rhs.z()
    };
  }

  /**
   * Perform a component-wise summation of this quaternion with another quaternion.
   * Store the result in this quaternion.
   *
   * @param rhs the other quaternion
   * @return a reference to this quaternion
   */
  auto operator+=(Derived const& rhs) -> Derived& {
    for (auto i : {0, 1, 2, 3}) data[i] += rhs[i];

    return *static_cast<Derived*>(this);
  }

  /**
   * Perform a component-wise subtraction of another quaternion from this quaternion.
   * Store the result in a new quaternion.
   *
   * @param rhs the other quaternion
   * @return the result quaternion
   */
  auto operator-(Derived const& rhs) const -> Derived {
    return Derived{
      w() - rhs.w(),
      x() - rhs.x(),
      y() - rhs.y(),
      z() - rhs.z()
    };
  }

  /**
   * Perform a component-wise subtraction of another quaterion from this quaternion.
   * Store the result in this quaternion.
   *
   * @param rhs the other quaternion
   * @return a reference to this quaternion
   */
  auto operator-=(Derived const& rhs) -> Derived& {
    for (auto i : {0, 1, 2, 3}) data[i] -= rhs[i];

    return *static_cast<Derived*>(this);
  }

  /**
   * Multiply each component of this quaternion with a scalar value.
   * Store the result in a new quaternion.
   *
   * @param scale the scalar value
   * @return the new quaternion
   */
  auto operator*(T scale) const -> Derived {
    return Derived{
      w() * scale,
      x() * scale,
      y() * scale,
      z() * scale
    };
  }

  /**
   * Multiply each component of this quaternion with a scalar value.
   * Store the result in this quaternion.
   *
   * @param scale the scalar value
   * @return a reference to this quaternion
   */
  auto operator*=(T scale) -> Derived& {
    for (auto i : {0, 1, 2, 3}) data[i] *= scale;

    return *static_cast<Derived*>(this);
  }

  /**
   * Calculate the Hamilton product of two quaternions.
   * Store the result in a new quaternion.
   * This operation is not commutative.
   * See: https://en.wikipedia.org/wiki/Quaternion#Hamilton_product
   *
   * @param rhs the right-hand side quaternion
   * @return the result quaternion
   */
  auto operator*(Derived const& rhs) const -> Derived {
    return Derived{
      w() * rhs.w() - x() * rhs.x() - y() * rhs.y() - z() * rhs.z(),
      x() * rhs.w() + w() * rhs.x() - z() * rhs.y() + y() * rhs.z(),
      y() * rhs.w() + z() * rhs.x() + w() * rhs.y() - x() * rhs.z(),
      z() * rhs.w() - y() * rhs.x() + x() * rhs.y() + w() * rhs.z()
    };
  }

  /**
   * Calculate the conjugate of this quaternion.
   * For rotation quaternions the conjugate is equivalent to the inverse.
   * @return the conjugate quaternion
   */
  auto operator~() const -> Derived {
    return Derived{*static_cast<Derived const*>(this)}.conjugate();
  }

  /**
   * Conjugate this quaternion.
   * For rotation quaternions the conjugate is equivalent to the inverse.
   * @return a reference to this quaternion.
   */
  auto conjugate() -> Derived& {
    x() = -x();
    y() = -y();
    z() = -z();
    return *static_cast<Derived*>(this);
  }

  /**
   * Calculate the inverse of this quaternion.
   * @return the inverse quaternion
   */
  auto inverse() const -> Derived {
    return ~(*this) * (NumericConstants<T>::one() / length_squared());
  }

  /**
   * Calculate the squared length of this quaternion.
   * @return the squared length
   */
  auto length_squared() const -> T {
    return w() * w() + x() * x() + y() * y() + z() * z();
  }

  /**
   * Calculate the dot product of this quaternion with another quaternion.
   * @return the dot product
   */
  auto dot(Derived const& rhs) const -> T {
    return w() * rhs.w() +
           x() * rhs.x() +
           y() * rhs.y() +
           z() * rhs.z();
  }

  /**
   * Calculate the cross-product of this quaternion's and another quaternion's vector components.
   *
   * @param rhs the right-hand side quaternion
   * @return the result quaternion
   */
  auto cross(Derived const& rhs) const -> Derived {
    return Derived{
      NumericConstants<T>::zero(),
      y() * rhs.z() - z() * rhs.y(),
      z() * rhs.x() - x() * rhs.z(),
      x() * rhs.y() - y() * rhs.x()
    };
  }

private:
  T data[4] {
    NumericConstants<T>::one(),
    NumericConstants<T>::zero(),
    NumericConstants<T>::zero(),
    NumericConstants<T>::zero()
  }; /**< the four components of this quaternion. */
};

} // namespace Aura::detail

/**
 * A float quaternion
 */
struct Quaternion final : detail::Quaternion<Quaternion, Vector3, float> {
  using detail::Quaternion<Quaternion, Vector3, float>::Quaternion;

  /**
   * Calculate the euclidean length of this quaternion.
   * @return the euclidean length
   */
  auto length() const -> float {
    return std::sqrt(length_squared());
  }

  /**
   * Normalize this quaternion.
   * @return a reference to this quaternion
   */
  auto normalize() -> Quaternion& {
    auto scale = 1.0 / length();
    *this *= scale;
    return *this;
  }

  /**
   * Create a 4x4 matrix equivalent to the rotation of this quaternion.
   * The result is only valid if this is a pure, normalised rotation quaternion.
   * @return the rotation matrix
   */
  auto to_rotation_matrix() const -> Matrix4 {
    /*
     * To derive this formula apply quaternion rotation on each
     * basis vector of the 3x3 identity matrix.
     *
     * mat[0] = Im(q * (0 1 0 0) * ~q)  (~q is the conjugate of q)
     * mat[1] = Im(q * (0 0 1 0) * ~q)
     * mat[2] = Im(q * (0 0 0 1) * ~q)
     */
    auto wx = w() * x();
    auto wy = w() * y();
    auto wz = w() * z();

    auto xx = x() * x();
    auto xy = x() * y();
    auto xz = x() * z();

    auto yy = y() * y();
    auto yz = y() * z();

    auto zz = z() * z();

    auto mat = Matrix4{};

    mat.x() = Vector4{
      1 - 2 * (zz + yy),
      2 * (xy + wz),
      2 * (xz - wy),
      0
    };

    mat.y() = Vector4{
      2 * (xy - wz),
      1 - 2 * (xx + zz),
      2 * (yz + wx),
      0
    };

    mat.z() = Vector4{
      2 * (xz + wy),
      2 * (yz - wx),
      1 - 2 * (xx + yy),
      0
    };

    mat.w().w() = 1;
    return mat;
  }

  /**
   * Create a quaternion from a rotation matrix.
   *
   * @param mat the rotation matrix
   * @return the quaternion
   */
  static auto from_rotation_matrix(Matrix4 const& mat) -> Quaternion {
    /*
     * Thanks to Martin John Baker from euclideanspace.com:
     *   http://www.euclideanspace.com/maths/geometry/rotations/conversions/matrixToQuaternion/index.htm
     *
     * Note that we use column-major indices unlike the original code.
     */
    auto m00 = mat[0][0];
    auto m11 = mat[1][1];
    auto m22 = mat[2][2];
    auto trace = m00 + m11 + m22;

    if (trace > 0.0) {
      auto s = std::sqrt(trace + 1) * 2;
      auto s_inv = 1 / s;
      return Quaternion{
        0.25f * s,
        (mat[1][2] - mat[2][1]) * s_inv,
        (mat[2][0] - mat[0][2]) * s_inv,
        (mat[0][1] - mat[1][0]) * s_inv
      };
    } else if (m00 > m11 && m00 > m22) {
      auto s = std::sqrt(1 + m00 - m11 - m22) * 2;
      auto s_inv = 1 / s;
      return Quaternion{
        (mat[1][2] - mat[2][1]) * s_inv,
        0.25f * s,
        (mat[1][0] + mat[0][1]) * s_inv,
        (mat[2][0] + mat[0][2]) * s_inv
      };
    } else if (m11 > m22) {
      auto s = std::sqrt(1 + m11 - m00 - m22) * 2;
      auto s_inv = 1 / s;
      return Quaternion{
        (mat[2][0] - mat[0][2]) * s_inv,
        (mat[1][0] + mat[0][1]) * s_inv,
        0.25f * s,
        (mat[2][1] + mat[1][2]) * s_inv
      };
    } else {
      auto s = std::sqrt(1 + m22 - m00 - m11) * 2;
      auto s_inv = 1 / s;
      return Quaternion{
        (mat[0][1] - mat[1][0]) * s_inv,
        (mat[2][0] + mat[0][2]) * s_inv,
        (mat[2][1] + mat[1][2]) * s_inv,
        0.25f * s
      };
    }
  }

  /**
   * Create a rotation quaternion from a rotational axis and angle.
   * 
   * @param axis the rotational axis
   * @param angle the angle (in radians)
   * @return the rotation quaternion
   */
  static auto from_axis_angle(Vector3 const& axis, float angle) -> Quaternion {
    auto a = angle * 0.5f;
    auto c = std::cos(a);
    auto s = std::sin(a);

    return Quaternion{c, axis.x() * s, axis.y() * s, axis.z() * s};
  }

  /**
   * Perform a simple linear interpolation between two quaternions with a factor between `0` and `1`.
   * The rotation property is not preserved.
   * Use {@link #nlerp} or {@link #slerp} instead to interpolate rotations.
   * 
   * @param q0 the quaternion at `factor = 0`
   * @param q1 the quaternion at `factor = 1`
   * @return the interpolated quaternion
   */
  static auto lerp(
    Quaternion const& q0,
    Quaternion const& q1,
    float t
  ) -> Quaternion {
    return q0 + (q1 - q0) * t;
  }

  /**
   * Perform a normalised linear interpolation between two quaternions with a factor between `0` and `1`.
   * This interpolation method is torque-minimal and variable velocity.
   *
   * @param q0 the quaternion at `factor = 0`
   * @param q1 the quaternion at `factor = 1`
   * @return the interpolated quaternion
   */
  static auto nlerp(
    Quaternion const& q0,
    Quaternion const& q1,
    float t
  ) -> Quaternion {
    return lerp(q0, q1, t).normalize();
  }

  /**
   * Perform a spherical interpolation between two quaternions with a factor between `0` and `1`.
   * This interpolation method is torque-minimal and constant velocity.
   *
   * @param q0 the quaternion at `factor = 0`
   * @param q1 the quaternion at `factor = 1`
   * @return the interpolated quaternion
   */
  static auto slerp(
    Quaternion const& q0,
    Quaternion const& q1,
    float t
  ) -> Quaternion {
    /*
     * Thanks to Jonathan Blow for this beautiful derivation of slerp:
     *   http://number-none.com/product/Understanding%20Slerp,%20Then%20Not%20Using%20It/
     *
     * TODO:
     *   - evaluate if the t=0 and t=1 checks are worth it.
     *   - evaluate if we really need to clamp cos_theta
     */
    if (t == 0.0) return q0;
    if (t == 1.0) return q1;

    auto cos_theta = q0.dot(q1);

    if (cos_theta > 0.9995) {
      return nlerp(q0, q1, t);
    }

    cos_theta = std::clamp(cos_theta, -1.0f, +1.0f);

    auto theta = std::acos(cos_theta);
    auto theta_t = theta * t;
    auto q2 = (q1 - q0 * cos_theta).normalize();

    return q0 * std::cos(theta_t) + q2 * std::sin(theta_t);
  }
};

} // namespace Aura
