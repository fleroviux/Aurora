/*
 * Copyright (C) 2021 fleroviux
 */

#pragma once

#include <aurora/scene/component/transform.hpp>
#include <aurora/scene/component.hpp>
#include <aurora/log.hpp>
#include <aurora/utility.hpp>
#include <algorithm>
#include <string>
#include <typeindex>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace Aura {

struct GameObject final : NonCopyable, NonMovable {
  GameObject() {
    add_default_components();
  }

  GameObject(std::string const& name) {
    add_default_components();
    set_name(name);
  }

 ~GameObject() {
    for (auto child : children()) delete child;
    for (auto pair : components_) delete pair.second;
  }

  bool has_parent() const {
    return parent() != nullptr;
  }

  auto parent() const -> GameObject* {
    return parent_;
  }

  auto children() const -> std::vector<GameObject*> const& {
    return children_;
  }

  auto transform() const -> Transform const& {
    return *transform_;
  }

  auto transform() -> Transform& {
    return *transform_;
  }

  auto get_name() const -> std::string const& {
    return name_;
  }

  void set_name(std::string const& name) {
    name_ = name;
  }

  bool visible() const {
    return visible_;
  }

  bool& visible() {
    return visible_;
  }

  void add_child(GameObject* child) {
    if (child->parent_ != nullptr) {
      if (child->parent_ == this) {
        return;
      }
      child->parent_->remove_child(child);
    }

    children_.push_back(child);
    child->parent_ = this;
  }

  void remove_child(GameObject* child) {
    if (child->parent_ == this) {
      children_.erase(std::find(children_.begin(), children_.end(), child));
      child->parent_ = nullptr;
    }
  }

  template<typename T>
  bool has_component() const {
    return get_component<T>() != nullptr;
  }

  template<typename T, typename... Args>
  auto add_component(Args&&... args) -> T* {
    AURA_ASSERT_IS_COMPONENT(T);

    auto component = new T{this, args...};
    Assert(!has_component<T>(),
      "AuroraScene: duplicate component {} on object {}.", typeid(T).name(), get_name());
    components_[std::type_index{typeid(T)}] = component;
    return component;
  }

  template<typename T>
  void remove_component() {
    AURA_ASSERT_IS_REMOVABLE_COMPONENT(T);

    auto it = components_.find(std::type_index{typeid(T)});
    if (it != components_.end()) {
      auto component = it->second;
      components_.erase(it);
      delete component;
    }
  }

  template<typename T>
  auto get_component() -> T* {
    AURA_ASSERT_IS_COMPONENT(T);
  
    auto it = components_.find(std::type_index{typeid(T)});
    if (it != components_.end()) {
      return reinterpret_cast<T*>(it->second);
    }
    return nullptr;
  }

  template<typename T>
  auto get_component() const -> T const* {
    return const_cast<GameObject*>(this)->get_component<T>();
  }

private:
  void add_default_components() {
    transform_ = add_component<Transform>();
  }

  GameObject* parent_ = nullptr;
  std::vector<GameObject*> children_;
  std::string name_ = "GameObject";
  bool visible_ = true;
  std::unordered_map<std::type_index, Component*> components_;
  Transform* transform_;
};

} // namespace Aura
