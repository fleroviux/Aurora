/*
 * Copyright (C) 2021 fleroviux
 */

#include <aurora/renderer/renderer.hpp>
#include <cstring>
#include <SDL.h>

#include "gltf_loader.hpp"

using namespace Aura;

auto create_example_scene() -> GameObject* {
  auto scene = new GameObject{};

  const u16 plane_indices[] = {
    0, 1, 2,
    2, 3, 0
  };

  const float plane_vertices[] = {
  // POSITION   UV         COLOR
    -1, +1, 2,  0.0, 0.0,  1.0, 0.0, 0.0,
    +1, +1, 2,  1.0, 0.0,  0.0, 1.0, 0.0,
    +1, -1, 2,  1.0, 1.0,  0.0, 0.0, 1.0,
    -1, -1, 2,  0.0, 1.0,  1.0, 0.0, 1.0
  };

  auto index_buffer = IndexBuffer{IndexDataType::UInt16, 6};
  std::memcpy(
    index_buffer.data(), plane_indices, sizeof(plane_indices));

  auto vertex_buffer_layout = VertexBufferLayout{
    .stride = sizeof(float) * 8,
    .attributes = {{
      .index = 0,
      .data_type = VertexDataType::Float32,
      .components = 3,
      .normalized = false,
      .offset = 0
    }, {
      .index = 2,
      .data_type = VertexDataType::Float32,
      .components = 2,
      .normalized = false,
      .offset = sizeof(float) * 3
    }, {
      .index = 3,
      .data_type = VertexDataType::Float32,
      .components = 3,
      .normalized = false,
      .offset = sizeof(float) * 5
    }}
  };
  auto vertex_buffer = VertexBuffer{vertex_buffer_layout, 4};
  std::memcpy(
    vertex_buffer.data(), plane_vertices, sizeof(plane_vertices));

  auto geometry = std::make_shared<Geometry>(index_buffer);
  geometry->buffers.push_back(std::move(vertex_buffer));

  auto material = std::make_shared<Material>();

  auto plane0 = new GameObject{"Plane0"};
  auto plane1 = new GameObject{"Plane1"};
  auto plane2 = new GameObject{"Plane2"};
  plane0->add_component<MeshComponent>(geometry, material);
  plane1->add_component<MeshComponent>(geometry, material);
  plane2->add_component<MeshComponent>(geometry, material);
  plane0->add_child(plane1);
  scene->add_child(plane0);
  scene->add_child(plane2);
  return scene;
}

int main() {
  SDL_Init(SDL_INIT_VIDEO);

  auto window = SDL_CreateWindow(
    "Aurora",
    SDL_WINDOWPOS_CENTERED,
    SDL_WINDOWPOS_CENTERED,
    1600,
    900,
    SDL_WINDOW_OPENGL
  );

  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

  auto gl_context = SDL_GL_CreateContext(window);

  glewInit();

  glClearColor(0.1, 0.1, 0.1, 1.0);
  glClear(GL_COLOR_BUFFER_BIT);

  auto renderer = OpenGLRenderer{};
  auto scene = create_example_scene();
  auto camera = new GameObject{"Camera"};
  scene->add_child(camera);

  auto gltf_loader = GLTFLoader{};
  auto cyoob = gltf_loader.parse("behemoth_sane/scene.gltf");
  //cyoob->transform().scale() = Vector3{0.05, 0.05, 0.05};
  cyoob->transform().rotation().x() = -3.141592*0.5;
  scene->add_child(cyoob);

  auto event = SDL_Event{};

  for (;;) {
    auto plane0 = scene->children()[0];
    auto plane1 = plane0->children()[0];
    auto plane2 = scene->children()[1];

    plane0->transform().rotation().z() += 0.01;
    plane1->transform().position().x() =  2.5;
    plane2->transform().position().x() = -2.5;

    auto state = SDL_GetKeyboardState(nullptr);

    if (state[SDL_SCANCODE_W]) camera->transform().position().y() += 0.05;
    if (state[SDL_SCANCODE_S]) camera->transform().position().y() -= 0.05;
    if (state[SDL_SCANCODE_A]) camera->transform().position().x() -= 0.05;
    if (state[SDL_SCANCODE_D]) camera->transform().position().x() += 0.05;
    if (state[SDL_SCANCODE_R]) camera->transform().position().z() += 0.05;
    if (state[SDL_SCANCODE_F]) camera->transform().position().z() -= 0.05;

    if (state[SDL_SCANCODE_UP])    camera->transform().rotation().x() -= 0.01;
    if (state[SDL_SCANCODE_DOWN])  camera->transform().rotation().x() += 0.01;
    if (state[SDL_SCANCODE_LEFT])  camera->transform().rotation().y() -= 0.01;
    if (state[SDL_SCANCODE_RIGHT]) camera->transform().rotation().y() += 0.01;

    renderer.render(scene, camera);
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
