/*
 * Copyright (C) 2021 fleroviux
 */

#include <aurora/scene/game_object.hpp>

namespace Aura {

void TransformComponent::update_local() {
  auto scale = Matrix4::scale(this->scale());

  auto rotation_x = Matrix4::rotation_x(rotation().x());
  auto rotation_y = Matrix4::rotation_y(rotation().y());
  auto rotation_z = Matrix4::rotation_z(rotation().z());
  auto rotation = rotation_z * rotation_y * rotation_x;

  auto translation = Matrix4::translation(position());

  matrix_local_ = translation * rotation * scale;
}

void TransformComponent::update_world(bool update_children) {
  if (owner()->has_parent()) {
    matrix_world_ = owner()->parent()->transform().world() * local();
  } else {
    matrix_world_ = matrix_local_;
  }

  if (update_children) {
    for (auto child : owner()->children()) {
      child->transform().update_world(update_children);
    }
  }
}

} // namespace Aura
