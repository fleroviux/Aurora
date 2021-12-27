/*
 * Copyright (C) 2021 fleroviux
 */

#pragma once

#include <algorithm>
#include <aurora/math/numeric_traits.hpp>
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

  auto x() -> T& { return data[0]; }
  auto y() -> T& { return data[1]; }
  auto z() -> T& { return data[2]; }
  auto w() -> T& { return data[3]; }

  auto x() const -> T { return data[0]; }
  auto y() const -> T { return data[1]; }
  auto z() const -> T { return data[2]; }
  auto w() const -> T { return data[3]; }

  auto operator+(Derived const& rhs) const -> Derived {
    return Derived{
      x() + rhs.x(),
      y() + rhs.y(),
      z() + rhs.z(),
      w() + rhs.w()
    };
  }

  auto operator+=(Derived const& rhs) -> Derived& {
    for (auto i : {0, 1, 2, 3}) data[i] += rhs[i];

    return *static_cast<Derived*>(this);
  }

  auto operator-(Derived const& rhs) const -> Derived {
    return Derived{
      x() - rhs.x(),
      y() - rhs.y(),
      z() - rhs.z(),
      w() - rhs.w()
    };
  }

  auto operator-=(Derived const& rhs) -> Derived& {
    for (auto i : {0, 1, 2, 3}) data[i] -= rhs[i];

    return *static_cast<Derived*>(this);
  }

  auto operator*(T scale) const -> Derived {
    return Derived{
      x() * scale,
      y() * scale,
      z() * scale,
      w() * scale
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
     * (x0 + y0*i + z0*j + w0*k) * (x1 + y1*i + z1*j + w1*k) = 
     *
     * x0*x1   + x0*y1*i  + x0*z1*j  + x0*w1*k  + 
     * y0*x1*i + y0*y1*ii + y0*z1*ij + y0*w1*ik +
     * z0*x1*j + z0*y1*ji + z0*z1*jj + z0*w1*jk +
     * w0*x1*k + w0*y1*ki + w0*z1*kj + w0*w1*kk =
     *
     * x0*x1   + x0*y1*i  + x0*z1*j  + x0*w1*k  + 
     * y0*x1*i - y0*y1    + y0*z1*k  - y0*w1*j  +
     * z0*x1*j - z0*y1*k  - z0*z1    + z0*w1*i  +
     * w0*x1*k + w0*y1*j  - w0*z1*i  - w0*w1   = 
     *
     * (x0*x1 - y0*y1 - z0*z1 - w0*w1) +
     * (y0*x1 + x0*y1 - w0*z1 + z0*w1)*i +
     * (z0*x1 + w0*y1 + x0*z1 - y0*w1)*j +
     * (w0*x1 - z0*y1 + y0*z1 + x0*w1)*k 
     */
    return Derived{
      x() * rhs.x() - y() * rhs.y() - z() * rhs.z() - w() * rhs.w(),
      y() * rhs.x() + x() * rhs.y() - w() * rhs.z() + z() * rhs.w(),
      z() * rhs.x() + w() * rhs.y() + x() * rhs.z() - y() * rhs.w(),
      w() * rhs.x() - z() * rhs.y() + y() * rhs.z() + x() * rhs.w()
    };
  }

  auto operator~() const -> Derived {
    return Derived{*static_cast<Derived const*>(this)}.conjugate();
  }

  auto conjugate() -> Derived& {
    y() = -y();
    z() = -z();
    w() = -w();
    return *static_cast<Derived*>(this);
  }

  auto dot(Derived const& rhs) const -> T {
    return x() * rhs.x() +
           y() * rhs.y() +
           z() * rhs.z() +
           w() * rhs.w();
  }

  auto cross(Derived const& rhs) const -> Derived {
    return Derived{
      NumericConstants<T>::zero(),
      z() * rhs.w() - w() * rhs.z(),
      w() * rhs.y() - y() * rhs.w(),
      y() * rhs.z() - z() * rhs.y()
    };
  }

private:
  T data[4] {
    NumericConstants<T>::zero(),
    NumericConstants<T>::zero(),
    NumericConstants<T>::zero(),
    NumericConstants<T>::one()
  };
};

} // namespace Aura::detail

struct Quaternion : detail::Quaternion<Quaternion, float> {
  using detail::Quaternion<Quaternion, float>::Quaternion;

  auto length() const -> float {
    return std::sqrt(x() * x() + y() * y() + z() * z() + w() * w());
  }

  auto normalize() -> Quaternion& {
    auto scale = 1.0 / length();
    *this *= scale;
    return *this;
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
