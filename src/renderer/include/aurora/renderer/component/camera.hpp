/*
 * Copyright (C) 2021 fleroviux
 */

#pragma once

#include <aurora/log.hpp>

#include <aurora/math/frustum.hpp>
#include <aurora/math/matrix4.hpp>
#include <aurora/scene/component.hpp>

namespace Aura {

struct Camera : Component {
  using Component::Component;

  virtual auto get_frustum() const -> Frustum const& = 0;
  virtual auto get_projection() const -> Matrix4 const& = 0;
};

struct PerspectiveCamera final : Camera {
  PerspectiveCamera(GameObject* owner) : Camera(owner) {
    update();
  }

  PerspectiveCamera(
    GameObject* owner,
    float field_of_view,
    float aspect_ratio,
    float near,
    float far 
  )   : Camera(owner)
      , field_of_view(field_of_view)
      , aspect_ratio(aspect_ratio)
      , near(near)
      , far(far) {
    update();
  }

  void set(float field_of_view, float aspect_ratio, float near, float far) {
    this->field_of_view = field_of_view;
    this->aspect_ratio = aspect_ratio;
    this->near = near;
    this->far = far;
    update();
  }

  auto get_field_of_view() const -> float {
    return field_of_view;
  }

  void set_field_of_view(float value) {
    field_of_view = value;
    update();
  }

  auto get_aspect_ratio() const -> float {
    return aspect_ratio;
  }

  void set_aspect_ratio(float value) {
    aspect_ratio = value;
    update();
  }

  auto get_near() const -> float {
    return near;
  }

  void set_near(float value) {
    near = value;
    update();
  }

  auto get_far() const -> float {
    return far;
  }

  void set_far(float value) {
    far = value;
    update();
  }

  auto get_frustum() const -> Frustum const& override {
    return frustum;
  }

  auto get_projection() const -> Matrix4 const& override {
    return projection;
  }

private:
  void update() {
    projection = Matrix4::PerspectiveVK(field_of_view, aspect_ratio, near, far);

    float x = 1 / projection.X().X();
    float y = 1 / projection.Y().Y();

    frustum.SetPlane(Frustum::Side::NZ, Plane{Vector3{ 0,  0, -1}, -near});
    frustum.SetPlane(Frustum::Side::PZ, Plane{Vector3{ 0,  0,  1}, -far });
    frustum.SetPlane(Frustum::Side::NX, Plane{Vector3{ 1 , 0, -x}.Normalize(), 0});
    frustum.SetPlane(Frustum::Side::PX, Plane{Vector3{-1 , 0, -x}.Normalize(), 0});
    frustum.SetPlane(Frustum::Side::NY, Plane{Vector3{ 0,  1, -y}.Normalize(), 0});
    frustum.SetPlane(Frustum::Side::PY, Plane{Vector3{ 0, -1, -y}.Normalize(), 0});
  }

  float field_of_view = 45.0;
  float aspect_ratio = 16 / 9.0;
  float near = 0.01;
  float far = 500.0;
  Frustum frustum;
  Matrix4 projection;
};

struct OrthographicCamera final : Camera {
  OrthographicCamera(GameObject* owner) : Camera(owner) {
    update();
  }

  OrthographicCamera(
    GameObject* owner,
    float left,
    float right,
    float bottom,
    float top,
    float near,
    float far
  )   : Camera(owner)
      , left(left)
      , right(right)
      , bottom(bottom)
      , top(top)
      , near(near)
      , far(far) {
    update();
  }

  void set(float left, float right, float bottom, float top, float near, float far) {
    this->left = left;
    this->right = right;
    this->bottom = bottom;
    this->top = top;
    this->near = near;
    this->far = far;
    update();
  }

  auto get_left() const -> float {
    return left;
  }

  void set_left(float value) {
    left = value;
    update();
  }

  auto get_right() const -> float {
    return right;
  }

  void set_right(float value) {
    right = value;
    update();
  }

  auto get_bottom() const -> float {
    return bottom;
  }

  void set_bottom(float value) {
    bottom = value;
    update();
  }

  auto get_top() const -> float {
    return top;
  }

  void set_top(float value) {
    top = value;
    update();
  }

  auto get_near() const -> float {
    return near;
  }

  void set_near(float value) {
    near = value;
    update();
  }

  auto get_far() const -> float {
    return far;
  }

  void set_far(float value) {
    far = value;
    update();
  }

  auto get_frustum() const -> Frustum const& override {
    return frustum;
  }

  auto get_projection() const -> Matrix4 const& override {
    return projection;
  }

private:
  void update() {
    frustum.SetPlane(Frustum::Side::NZ, Plane{Vector3{ 0,  0, -1}, -near});
    frustum.SetPlane(Frustum::Side::PZ, Plane{Vector3{ 0,  0,  1}, -far});
    frustum.SetPlane(Frustum::Side::NX, Plane{Vector3{ 1,  0,  0},  left});
    frustum.SetPlane(Frustum::Side::PX, Plane{Vector3{-1,  0,  0},  right});
    frustum.SetPlane(Frustum::Side::NY, Plane{Vector3{ 0,  1, -1},  bottom});
    frustum.SetPlane(Frustum::Side::PY, Plane{Vector3{ 0, -1, -1},  top});

    // TODO: fix the depth range
    projection = Matrix4::OrthographicGL(left, right, bottom, top, near, far);
  }

  float left = -1.0;
  float right = 1.0;
  float bottom = -1.0;
  float top = 1.0;
  float near = 0.01;
  float far = 500.0;
  Frustum frustum;
  Matrix4 projection;
};

} // namespace Aura
