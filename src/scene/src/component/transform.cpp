/*
 * Copyright (C) 2021 fleroviux
 */

#include <aurora/math/quaternion.hpp>
#include <aurora/scene/game_object.hpp>

namespace Aura {

void Transform::update_local() {
  matrix_local_ = rotation_.get_matrix();

  matrix_local_.x() *= scale().x();
  matrix_local_.y() *= scale().y();
  matrix_local_.z() *= scale().z();
  matrix_local_.w()  = Vector4{position(), 1.0};
}

void Transform::update_world(bool update_children) {
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
