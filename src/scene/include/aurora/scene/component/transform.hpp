/*
 * Copyright (C) 2021 fleroviux
 */

#include <aurora/math/matrix4.hpp>
#include <aurora/math/rotator.hpp>
#include <aurora/scene/component.hpp>

namespace Aura {

/**
 * TODO:
 * - Think about a better interface for matrix updated
 */

struct TransformComponent final : Component {
  TransformComponent(GameObject* owner) : Component(owner) {}

  auto position() -> Vector3& {
    return position_;
  }

  auto position() const -> Vector3 const& {
    return position_;
  }

  auto rotation() -> Rotator& {
    return rotation_;
  }

  auto rotation() const -> Rotator const& {
    return rotation_;
  }

  auto scale() -> Vector3& {
    return scale_;
  }

  auto scale() const -> Vector3 const& {
    return scale_;
  }

  auto local() const -> Matrix4 const& {
    return matrix_local_;
  }

  auto world() const -> Matrix4 const& {
    return matrix_world_;
  }

  auto auto_update() -> bool& {
    return auto_update_;
  }

  void update_local();
  void update_world(bool update_children);

  static constexpr bool removable() {
    return false;
  }

private:
  Vector3 position_ {0, 0, 0};
  Rotator rotation_;
  Vector3 scale_ {1, 1, 1};

  bool auto_update_ = true;
  Matrix4 matrix_local_;
  Matrix4 matrix_world_;
};

} // namespace Aura
