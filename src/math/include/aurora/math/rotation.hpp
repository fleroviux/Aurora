/*
 * Copyright (C) 2021 fleroviux
 */

#pragma once

#include <aurora/math/quaternion.hpp>

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
    auto sin_y = -mat[0][2];
    auto y = std::asin(std::clamp(sin_y, -1.0f, +1.0f));
    auto cos_y = std::sqrt(1 - sin_y * sin_y);

    auto sin_x = mat[1][2] / cos_y;
    auto cos_x = mat[2][2] / cos_y;
    auto x = std::atan2(sin_x, cos_x);

    auto sin_z = mat[0][1] / cos_y;
    auto cos_z = mat[0][0] / cos_y;
    auto z = std::atan2(sin_z, cos_z);

    return Vector3{x, y, z};
  }

  void set_euler(Vector3 const& euler) {
    set_euler(euler.x(), euler.y(), euler.z());
  }

  void set_euler(float x, float y, float z) {
    auto half_x = x * 0.5;
    auto cos_x = std::cos(half_x);
    auto sin_x = std::sin(half_x);
    
    auto half_y = y * 0.5;
    auto cos_y = std::cos(half_y);
    auto sin_y = std::sin(half_y);
    
    auto half_z = z * 0.5;
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
    mat = quat.to_rotation_matrix();
  }

  Matrix4 mat;
  Quaternion quat;
};

} // namespace Aura
