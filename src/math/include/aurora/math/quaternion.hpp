/*
 * Copyright (C) 2021 fleroviux
 */

#pragma once

#include <algorithm>
#include <aurora/math/numeric_traits.hpp>
#include <aurora/math/matrix4.hpp>
#include <cmath>
#include <initializer_list>

namespace Aura {

namespace detail {

template<typename Derived, typename T>
struct Quaternion {
  Quaternion() {}

  Quaternion(T x, T y, T z, T w) : data{x, y, z, w} {}

  auto operator[](int index) -> T& {
    return data[index];
  }

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

  auto operator+(Derived const& rhs) const -> Derived {
    return Derived{
      w() + rhs.w(),
      x() + rhs.x(),
      y() + rhs.y(),
      z() + rhs.z()
    };
  }

  auto operator+=(Derived const& rhs) -> Derived& {
    for (auto i : {0, 1, 2, 3}) data[i] += rhs[i];

    return *static_cast<Derived*>(this);
  }

  auto operator-(Derived const& rhs) const -> Derived {
    return Derived{
      w() - rhs.w(),
      x() - rhs.x(),
      y() - rhs.y(),
      z() - rhs.z()
    };
  }

  auto operator-=(Derived const& rhs) -> Derived& {
    for (auto i : {0, 1, 2, 3}) data[i] -= rhs[i];

    return *static_cast<Derived*>(this);
  }

  auto operator*(T scale) const -> Derived {
    return Derived{
      w() * scale,
      x() * scale,
      y() * scale,
      z() * scale
    };
  }

  auto operator*=(T scale) -> Derived& {
    for (auto i : {0, 1, 2, 3}) data[i] *= scale;

    return *static_cast<Derived*>(this);
  }

  auto operator*(Derived const& rhs) const -> Derived {
    /*
     * i^2 = j^2 = k^2 = ijk = -1
     *
     * ij =  k
     * ik = -j
     * ji = -k
     * jk =  i
     * ki =  j
     * kj = -i
     *
     * (w0 + x0*i + y0*j + z0*k) * (w1 + x1*i + y1*j + w1*k) = 
     *
     * w0*w1   + w0*x1*i  + w0*y1*j  + w0*w1*k  + 
     * x0*w1*i + x0*x1*ii + x0*y1*ij + x0*w1*ik +
     * y0*w1*j + y0*x1*ji + y0*y1*jj + y0*w1*jk +
     * z0*w1*k + z0*x1*ki + z0*y1*kj + z0*w1*kk =
     *
     * w0*w1   + w0*x1*i  + w0*y1*j  + w0*w1*k  + 
     * x0*w1*i - x0*x1    + x0*y1*k  - x0*w1*j  +
     * y0*w1*j - y0*x1*k  - y0*y1    + y0*w1*i  +
     * z0*w1*k + z0*x1*j  - z0*y1*i  - z0*w1   = 
     *
     * (w0*w1 - x0*x1 - y0*y1 - z0*w1) +
     * (x0*w1 + w0*x1 - z0*y1 + y0*w1)*i +
     * (y0*w1 + z0*x1 + w0*y1 - x0*w1)*j +
     * (z0*w1 - y0*x1 + x0*y1 + w0*w1)*k 
     */
    return Derived{
      w() * rhs.w() - x() * rhs.x() - y() * rhs.y() - z() * rhs.z(),
      x() * rhs.w() + w() * rhs.x() - z() * rhs.y() + y() * rhs.z(),
      y() * rhs.w() + z() * rhs.x() + w() * rhs.y() - x() * rhs.z(),
      z() * rhs.w() - y() * rhs.x() + x() * rhs.y() + w() * rhs.z()
    };
  }

  auto operator~() const -> Derived {
    return Derived{*static_cast<Derived const*>(this)}.conjugate();
  }

  auto conjugate() -> Derived& {
    x() = -x();
    y() = -y();
    z() = -z();
    return *static_cast<Derived*>(this);
  }

  auto dot(Derived const& rhs) const -> T {
    return w() * rhs.w() +
           x() * rhs.x() +
           y() * rhs.y() +
           z() * rhs.z();
  }

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
  };
};

} // namespace Aura::detail

struct Quaternion : detail::Quaternion<Quaternion, float> {
  using detail::Quaternion<Quaternion, float>::Quaternion;

  auto length() const -> float {
    return std::sqrt(w() * w() + x() * x() + y() * y() + z() * z());
  }

  auto normalize() -> Quaternion& {
    auto scale = 1.0 / length();
    *this *= scale;
    return *this;
  }

  auto to_rotation_matrix() const -> Matrix4 {
    auto aa = w() * w();
    auto bb = x() * x();
    auto cc = y() * y();
    auto dd = z() * z();

    auto two_ab = 2 * w() * x();
    auto two_ac = 2 * w() * y();
    auto two_ad = 2 * w() * z();
    auto two_bc = 2 * x() * y();
    auto two_bd = 2 * x() * z();
    auto two_cd = 2 * y() * z();

    auto mat = Matrix4{};

    mat.x() = Vector4{
      bb + aa - dd - cc,
      two_bc + two_ad,
      two_bd - two_ac,
      0
    };

    mat.y() = Vector4{
      two_bc - two_ad,
      cc - dd + aa - bb,
      two_cd + two_ab,
      0
    };

    mat.z() = Vector4{
      two_bd + two_ac,
      two_cd - two_ab,
      dd - cc - bb + aa,
      0
    };

    mat.w().w() = 1;
    return mat;
  }

  static auto lerp(
    Quaternion const& q0,
    Quaternion const& q1,
    float t
  ) -> Quaternion {
    return q0 + (q1 - q0) * t;
  }

  static auto nlerp(
    Quaternion const& q0,
    Quaternion const& q1,
    float t
  ) -> Quaternion {
    return lerp(q0, q1, t).normalize();
  }

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
