/*
 * Copyright (C) 2021 fleroviux
 */

#include <aurora/math/quaternion.hpp>
#include <aurora/scene/game_object.hpp>

namespace Aura {

void TransformComponent::update_local() {
  auto scale = Matrix4::scale(scale_);
  auto rotation = rotation_.get_matrix();
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
