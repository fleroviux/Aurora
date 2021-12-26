/*
 * Copyright (C) 2021 fleroviux
 */

#pragma once

#include <cmath>

namespace Aura {

struct Quaternion {
  Quaternion() {}

  Quaternion(float x, float y, float z, float w) : data{x, y, z, w} {}

  auto operator[](int index) -> float& {
    return data[index];
  }

  auto operator[](int index) const -> float {
    return data[index];
  }

  auto x() -> float& { return data[0]; }
  auto y() -> float& { return data[1]; }
  auto z() -> float& { return data[2]; }
  auto w() -> float& { return data[3]; }

  auto x() const -> float { return data[0]; }
  auto y() const -> float { return data[1]; }
  auto z() const -> float { return data[2]; }
  auto w() const -> float { return data[3]; }

  auto operator+(Quaternion const& rhs) const -> Quaternion {
    return Quaternion{
      x() + rhs.x(),
      y() + rhs.y(),
      z() + rhs.z(),
      w() + rhs.w()
    };
  }

  auto operator+=(Quaternion const& rhs) -> Quaternion& {
    x() += rhs.x();
    y() += rhs.y();
    z() += rhs.z();
    w() += rhs.w();
    return *this;
  }

  // auto operator-(Quaternion const& rhs) const -> Quaternion {
  //   return Quaternion{
  //     x() - rhs.x(),
  //     y() - rhs.y(),
  //     z() - rhs.z(),
  //     w() - rhs.w()
  //   };
  // }

  // auto operator-=(Quaternion const& rhs) -> Quaternion& {
  //   x() -= rhs.x();
  //   y() -= rhs.y();
  //   z() -= rhs.z();
  //   w() -= rhs.w();
  //   return *this;
  // }

  auto operator*(Quaternion const& rhs) const -> Quaternion {
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
    return Quaternion{
      x() * rhs.x() - y() * rhs.y() - z() * rhs.z() - w() * rhs.w(),
      y() * rhs.x() + x() * rhs.y() - w() * rhs.z() + z() * rhs.w(),
      z() * rhs.x() + w() * rhs.y() + x() * rhs.z() - y() * rhs.w(),
      w() * rhs.x() - z() * rhs.y() + y() * rhs.z() + x() * rhs.w()
    };
  }

  auto operator~() const -> Quaternion {
    return Quaternion{*this}.conjugate();
  }

  auto conjugate() -> Quaternion& {
    y() = -y();
    z() = -z();
    w() = -w();
    return *this;
  }

  auto dot(Quaternion const& rhs) const -> float {
    return x() * rhs.x() +
           y() * rhs.y() +
           z() * rhs.z() +
           w() * rhs.w();
  }

  auto cross(Quaternion const& rhs) const -> Quaternion {
    return Quaternion{
      0,
      z() * rhs.w() - w() * rhs.z(),
      w() * rhs.y() - y() * rhs.w(),
      y() * rhs.z() - z() * rhs.y()
    };
  }

  auto length() const -> float {
    return std::sqrt(x() * x() + y() * y() + z() * z() + w() * w());
  }

  auto normalize() -> Quaternion& {
    auto scale = 1.0 / length();
    x() *= scale;
    y() *= scale;
    z() *= scale;
    w() *= scale;
    return *this;
  }

private:
  float data[4] { 0.0 };
};

} // namespace Aura
