/*
 * Copyright (C) 2021 fleroviux
 */

#pragma once

#include <array>
#include <cmath>
#include <aurora/math/vector.hpp>

namespace Aura {

namespace detail {

// TODO: align data to 16-byte boundary to allow for better vectorization?
template<typename Derived, typename T>
struct Matrix4 {
  Matrix4() {};

  Matrix4(std::array<T, 16> const& elements) {
    for (uint i = 0; i < 16; i++)
      data[i & 3][i >> 2] = elements[i];
  }

  auto operator[](int i) -> Vector4<T>& {
    return data[i];
  }
  
  auto operator[](int i) const -> Vector4<T> const& {
    return data[i];
  }

  auto x() -> Vector4<T>& { return data[0]; }
  auto y() -> Vector4<T>& { return data[1]; }
  auto z() -> Vector4<T>& { return data[2]; }
  auto w() -> Vector4<T>& { return data[3]; }

  auto x() const -> Vector4<T> const& { return data[0]; }
  auto y() const -> Vector4<T> const& { return data[1]; }
  auto z() const -> Vector4<T> const& { return data[2]; }
  auto w() const -> Vector4<T> const& { return data[3]; }

  auto operator*(Vector4<T> const& vec) const -> Vector4<T> {
    Vector4<T> result{};
    for (uint i = 0; i < 4; i++)
      result += data[i] * vec[i];
    return result;
  }

  auto operator*(Derived const& other) const -> Derived {
    Derived result{};
    for (uint i = 0; i < 4; i++)
      result[i] = *this * other[i];
    return result;
  }

  auto inverse() const -> Derived {
    // Adapted from this code: https://stackoverflow.com/a/44446912
    auto a2323 = data[2][2] * data[3][3] - data[3][2] * data[2][3];
    auto a1323 = data[1][2] * data[3][3] - data[3][2] * data[1][3];
    auto a1223 = data[1][2] * data[2][3] - data[2][2] * data[1][3];
    auto a0323 = data[0][2] * data[3][3] - data[3][2] * data[0][3];
    auto a0223 = data[0][2] * data[2][3] - data[2][2] * data[0][3];
    auto a0123 = data[0][2] * data[1][3] - data[1][2] * data[0][3];
    auto a2313 = data[2][1] * data[3][3] - data[3][1] * data[2][3];
    auto a1313 = data[1][1] * data[3][3] - data[3][1] * data[1][3];
    auto a1213 = data[1][1] * data[2][3] - data[2][1] * data[1][3];
    auto a2312 = data[2][1] * data[3][2] - data[3][1] * data[2][2];
    auto a1312 = data[1][1] * data[3][2] - data[3][1] * data[1][2];
    auto a1212 = data[1][1] * data[2][2] - data[2][1] * data[1][2];
    auto a0313 = data[0][1] * data[3][3] - data[3][1] * data[0][3];
    auto a0213 = data[0][1] * data[2][3] - data[2][1] * data[0][3];
    auto a0312 = data[0][1] * data[3][2] - data[3][1] * data[0][2];
    auto a0212 = data[0][1] * data[2][2] - data[2][1] * data[0][2];
    auto a0113 = data[0][1] * data[1][3] - data[1][1] * data[0][3];
    auto a0112 = data[0][1] * data[1][2] - data[1][1] * data[0][2];

    auto det = data[0][0] * (data[1][1] * a2323 - data[2][1] * a1323 + data[3][1] * a1223) 
             - data[1][0] * (data[0][1] * a2323 - data[2][1] * a0323 + data[3][1] * a0223) 
             + data[2][0] * (data[0][1] * a1323 - data[1][1] * a0323 + data[3][1] * a0123) 
             - data[3][0] * (data[0][1] * a1223 - data[1][1] * a0223 + data[2][1] * a0123);

    auto recip_det = 1 / det;

    return Derived{{
      recip_det *  (data[1][1] * a2323 - data[2][1] * a1323 + data[3][1] * a1223),
      recip_det * -(data[1][0] * a2323 - data[2][0] * a1323 + data[3][0] * a1223),
      recip_det *  (data[1][0] * a2313 - data[2][0] * a1313 + data[3][0] * a1213),
      recip_det * -(data[1][0] * a2312 - data[2][0] * a1312 + data[3][0] * a1212),
      recip_det * -(data[0][1] * a2323 - data[2][1] * a0323 + data[3][1] * a0223),
      recip_det *  (data[0][0] * a2323 - data[2][0] * a0323 + data[3][0] * a0223),
      recip_det * -(data[0][0] * a2313 - data[2][0] * a0313 + data[3][0] * a0213),
      recip_det *  (data[0][0] * a2312 - data[2][0] * a0312 + data[3][0] * a0212),
      recip_det *  (data[0][1] * a1323 - data[1][1] * a0323 + data[3][1] * a0123),
      recip_det * -(data[0][0] * a1323 - data[1][0] * a0323 + data[3][0] * a0123),
      recip_det *  (data[0][0] * a1313 - data[1][0] * a0313 + data[3][0] * a0113),
      recip_det * -(data[0][0] * a1312 - data[1][0] * a0312 + data[3][0] * a0112),
      recip_det * -(data[0][1] * a1223 - data[1][1] * a0223 + data[2][1] * a0123),
      recip_det *  (data[0][0] * a1223 - data[1][0] * a0223 + data[2][0] * a0123),
      recip_det * -(data[0][0] * a1213 - data[1][0] * a0213 + data[2][0] * a0113),
      recip_det *  (data[0][0] * a1212 - data[1][0] * a0212 + data[2][0] * a0112)
    }};
  }

  static auto identity() -> Derived {
    Derived result{};

    for (uint row = 0; row < 4; row++) {
      for (uint col = 0; col < 4; col++) {
        if (row == col) {
          result[col][row] = NumericConstants<T>::one();
        } else {
          result[col][row] = NumericConstants<T>::zero();
        }
      }
    }

    return result;
  }

private:
  Vector4<T> data[4] {};
};

} // namespace Aura::detail

struct Matrix4 : detail::Matrix4<Matrix4, float> {
  Matrix4() : detail::Matrix4<Matrix4, float>() {}

  Matrix4(std::array<float, 16> const& elements) : detail::Matrix4<Matrix4, float>(elements) {}

  static auto scale(float x, float y, float z) -> Matrix4 {
    return Matrix4{{
      x, 0, 0, 0,
      0, y, 0, 0,
      0, 0, z, 0,
      0, 0, 0, 1
    }};
  }

  static auto scale(Vector3 const& vec) -> Matrix4 {
    return scale(vec.x(), vec.y(), vec.z());
  }

  static auto rotation_x(float radians) -> Matrix4 {
    auto cos = std::cos(radians);
    auto sin = std::sin(radians);

    return Matrix4{{
      1,   0,    0, 0,
      0, cos, -sin, 0,
      0, sin,  cos, 0,
      0,   0,    0, 1
    }};
  }

  static auto rotation_y(float radians) -> Matrix4 {
    auto cos = std::cos(radians);
    auto sin = std::sin(radians);

    return Matrix4{{
      cos, 0,  sin, 0,
      0,   1,    0, 0,
     -sin, 0,  cos, 0,
      0,   0,    0, 1
    }};
  }

  static auto rotation_z(float radians) -> Matrix4 {
    auto cos = std::cos(radians);
    auto sin = std::sin(radians);

    return Matrix4{{
      cos, -sin, 0, 0,
      sin,  cos, 0, 0,
        0,    0, 1, 0,
        0,    0, 0, 1
    }};
  }

  static auto translation(float x, float y, float z) -> Matrix4 {
    return Matrix4{{
      1, 0, 0, x,
      0, 1, 0, y,
      0, 0, 1, z,
      0, 0, 0, 1
    }};
  }

  static auto translation(Vector3 const& vec) -> Matrix4 {
    return Matrix4::translation(vec.x(), vec.y(), vec.z());
  }

  static auto perspective_gl(
    float fov_y,
    float aspect_ratio,
    float near,
    float far
  ) -> Matrix4 {
    // cot(fov_y/2) = tan((pi - fov_y)/2)
    auto y = std::tan((M_PI - fov_y) * 0.5);
    auto x = y / aspect_ratio;

    auto a = 1 / (near - far);
    auto b = (far + near) * a;
    auto c = (2 * far * near) * a;

    return Matrix4{{
      x, 0,  0, 0,
      0, y,  0, 0,
      0, 0,  b, c,
      0, 0, -1, 0
    }};
  }

  static auto perspective_dx(
    float fov_y,
    float aspect_ratio,
    float near,
    float far
  ) -> Matrix4 {
    // cot(fov_y/2) = tan((pi - fov_y)/2)
    auto y = std::tan((M_PI - fov_y) * 0.5);
    auto x = y / aspect_ratio;
    
    float a = 1.0 / (far - near);
    float b = far * a;
    float c = -near * far * a;

    return Matrix4{{
      x, 0, 0, 0,
      0, y, 0, 0,
      0, 0, b, c,
      0, 0, 1, 0
    }};
  }
};

} // namespace Aura
