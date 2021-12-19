/*
 * Copyright (C) 2021 fleroviux
 */

#include <aurora/scene/geometry/geometry.hpp>
#include <aurora/scene/game_object.hpp>
#include <cstring>
#include <SDL.h>
#include <GL/glew.h>

using namespace Aura;

struct MeshComponent final : Component {
  MeshComponent(GameObject* owner, Geometry* geometry)
      : Component(owner)
      , geometry(geometry) {
  }

  Geometry* geometry;
};

auto create_example_scene() -> GameObject* {
  auto scene = new GameObject{};
  auto plane = new GameObject{"Plane"};

  const u16 plane_indices[] = {
    0, 1, 2,
    1, 2, 3
  };

  const float plane_vertices[] = {
    -1, +1, 2,
    +1, +1, 2,
    +1, -1, 2,
    -1, -1, 2
  };

  auto index_buffer = IndexBuffer{IndexDataType::UInt16, 6};
  std::memcpy(
    index_buffer.data(), plane_indices, sizeof(plane_indices));

  auto vertex_buffer_layout = VertexBufferLayout{
    .stride = sizeof(float) * 3,
    .attributes = std::vector<VertexBufferLayout::Attribute>{VertexBufferLayout::Attribute{
      .data_type = VertexDataType::Float32,
      .components = 3,
      .normalized = false,
      .offset = 0
    }}
  };
  auto vertex_buffer = VertexBuffer{vertex_buffer_layout, 4};
  std::memcpy(
    vertex_buffer.data(), plane_vertices, sizeof(plane_vertices));

  auto geometry = new Geometry{index_buffer, vertex_buffer};
  plane->add_component<MeshComponent>(geometry);
  scene->add_child(plane);
  return scene;
}

int main() {
  SDL_Init(SDL_INIT_VIDEO);

  auto window = SDL_CreateWindow(
    "Aurora",
    SDL_WINDOWPOS_CENTERED,
    SDL_WINDOWPOS_CENTERED,
    1920,
    1080,
    SDL_WINDOW_OPENGL
  );

  auto gl_context = SDL_GL_CreateContext(window);

  glewInit();

  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

  glClearColor(0.1, 0.1, 0.1, 1.0);
  glClear(GL_COLOR_BUFFER_BIT);

  auto scene = create_example_scene();

  auto event = SDL_Event{};

  for (;;) {
    SDL_GL_SwapWindow(window);

    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) {
        goto done;
      }
    }
  }

done:
  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}
