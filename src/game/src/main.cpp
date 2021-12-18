/*
 * Copyright (C) 2021 fleroviux
 */

#include <aurora/log.hpp>
#include <aurora/math/matrix4.hpp>
#include <aurora/scene/game_object.hpp>
#include <functional>

using namespace Aura;

struct PoggerComponent final : Component {
  int pog_a = 1;
  int pog_b = 2;

  using Component::Component;

  PoggerComponent(GameObject* owner, int a, int b)
      : Component(owner)
      , pog_a(a)
      , pog_b(b) {
  }
};

void debug_dump_scene(GameObject* scene) {
  const std::function<void(GameObject*, int)> traverse = [&](GameObject* parent, int level) -> void {
    for (int i = 0; i < level; i++) fmt::print("  ");

    if (parent->has_component<PoggerComponent>()) {
      auto pogger = parent->get_component<PoggerComponent>();

      fmt::print("* {} (a={} b={})\n", parent->get_name(), pogger->pog_a, pogger->pog_b);
    } else {
      fmt::print("* {}\n", parent->get_name());
    }

    for (auto child : parent->children()) traverse(child, level+1);
  };

  fmt::print("Scene Dump:\n");

  traverse(scene, 0);
}

int main() {
  Log<Info>("AuroraGame: we are running :rpog:");

  auto scene = new GameObject{"Scene"};
  auto child_a = new GameObject{"A"};
  auto child_b = new GameObject{"B"};
  auto child_c = new GameObject{"C"};
  auto child_d = new GameObject{"D"};

  // create initial scene structure
  scene->add_child(child_a);
  child_a->add_child(child_b);
  child_b->add_child(child_c);
  child_c->add_child(child_d);

  debug_dump_scene(scene);

  // restructure scene
  child_b->add_child(child_d);
  scene->add_child(child_b);

  debug_dump_scene(scene);

  child_a->add_component<PoggerComponent>();
  child_b->add_component<PoggerComponent>(420, 69);
  child_c->add_component<PoggerComponent>(1337, 1337);

  debug_dump_scene(scene);

  child_b->remove_component<PoggerComponent>();

  debug_dump_scene(scene);

  child_a->get_component<TransformComponent>()->scale() = Vector3{-1, 1, 1};
  child_a->transform().position() = Vector3{1, 1, 1};
  child_a->transform().update_local();
  child_a->transform().update_world(true);
  return 0;
}
