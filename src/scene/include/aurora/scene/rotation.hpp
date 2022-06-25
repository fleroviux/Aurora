
// Copyright (C) 2022 fleroviux. All rights reserved.

#pragma once

#include <aurora/math/quaternion.hpp>
#include <aurora/log.hpp>

namespace Aura {

struct Rotation {
  Rotation() {
    calculate_matrix();
  }

  Rotation(float x, float y, float z) {
    set_euler(x, y, z);
  }

  Rotation(Quaternion const& quat) {
    set_quaternion(quat);
  }

  auto get_euler() const -> Vector3 {
    constexpr float cos0_threshold = 1.0 - 1e-6;

    auto euler = Vector3{};
    auto sin_y = -mat[0][2];

    euler.Y() = std::asin(std::clamp(sin_y, -1.0f, +1.0f));

    // Guard against gimbal lock when Y=-90°/+90° (X and Z rotate around the same axis).
    if (std::abs(sin_y) <= cos0_threshold) {
      auto sin_x_cos_y = mat[1][2];
      auto cos_x_cos_y = mat[2][2];
      euler.X() = std::atan2(sin_x_cos_y, cos_x_cos_y);

      auto sin_z_cos_y = mat[0][1];
      auto cos_z_cos_y = mat[0][0];
      euler.Z() = std::atan2(sin_z_cos_y, cos_z_cos_y);
    } else {
      euler.X() = std::atan2(mat[1][0], mat[2][0]);
      euler.Z() = 0;
    }

    return euler;
  }

  void set_euler(Vector3 const& euler) {
    set_euler(euler.X(), euler.Y(), euler.Z());
  }

  void set_euler(float x, float y, float z) {
    auto half_x = x * 0.5f;
    auto cos_x = std::cos(half_x);
    auto sin_x = std::sin(half_x);
    
    auto half_y = y * 0.5f;
    auto cos_y = std::cos(half_y);
    auto sin_y = std::sin(half_y);
    
    auto half_z = z * 0.5f;
    auto cos_z = std::cos(half_z);
    auto sin_z = std::sin(half_z);
    
    auto cos_z_cos_y = cos_z * cos_y;
    auto sin_z_cos_y = sin_z * cos_y;
    auto sin_z_sin_y = sin_z * sin_y;
    auto cos_z_sin_y = cos_z * sin_y;
    
    quat = Quaternion{
      cos_z_cos_y * cos_x + sin_z_sin_y * sin_x,
      cos_z_cos_y * sin_x - sin_z_sin_y * cos_x,
      sin_z_cos_y * sin_x + cos_z_sin_y * cos_x,
      sin_z_cos_y * cos_x - cos_z_sin_y * sin_x 
    };

    calculate_matrix();
  }

  auto get_quaternion() const -> Quaternion const& {
    return quat;
  }

  void set_quaternion(Quaternion const& quat) {
    this->quat = quat;
    calculate_matrix();
  }

  void set_quaternion(float w, float x, float y, float z) {
    set_quaternion(Quaternion{w, x, y, z});
  }

  void set_identity() {
    set_quaternion(Quaternion{});
  }

  auto get_matrix() const -> Matrix4 const& {
    return mat;
  }

private:
  void calculate_matrix() {
    mat = quat.ToRotationMatrix();
  }

  Matrix4 mat;
  Quaternion quat;
};

} // namespace Aura
