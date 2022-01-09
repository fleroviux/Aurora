/*
 * Copyright (C) 2021 fleroviux
 */

#include <aurora/math/rotation.hpp>
#include <aurora/renderer/renderer.hpp>
#include <aurora/renderer/uniform_block.hpp>
#include <cstring>
#include <SDL.h>

#ifdef main
  #undef main
#endif

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

  auto material = std::make_shared<PbrMaterial>();

  auto plane0 = new GameObject{"Plane0"};
  auto plane1 = new GameObject{"Plane1"};
  auto plane2 = new GameObject{"Plane2"};
  plane0->add_component<Mesh>(geometry, material);
  plane1->add_component<Mesh>(geometry, material);
  plane2->add_component<Mesh>(geometry, material);
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
    2560,
    1440,
    SDL_WINDOW_OPENGL
  );

  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
  SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 8);

  auto gl_context = SDL_GL_CreateContext(window);

  glewInit();

  glClearColor(0.1, 0.1, 0.1, 1.0);
  glClear(GL_COLOR_BUFFER_BIT);

  auto renderer = OpenGLRenderer{};
  auto scene = new GameObject{"Scene"};
  auto camera = new GameObject{"Camera"};
  //camera->transform().position() = Vector3{4.0, 4.0, -3.0};
  camera->add_component<PerspectiveCamera>();
  scene->add_component<Scene>(camera);
  scene->add_child(camera);

  auto behemoth = GLTFLoader{}.parse("behemoth/behemoth.gltf");
  scene->add_child(behemoth);

  auto sponza = GLTFLoader{}.parse("Sponza/Sponza.gltf");
  scene->add_child(sponza);

  auto event = SDL_Event{};

  float x = 0;
  float y = 0;
  float z = 0;

  for (;;) {
    auto state = SDL_GetKeyboardState(nullptr);

    auto const& camera_local = camera->transform().local();

    if (state[SDL_SCANCODE_W]) {
      camera->transform().position() -= camera_local[2].xyz() * 0.05;
    }

    if (state[SDL_SCANCODE_S]) {
      camera->transform().position() += camera_local[2].xyz() * 0.05;
    }

    if (state[SDL_SCANCODE_A]) {
      camera->transform().position() -= camera_local[0].xyz() * 0.05;
    }

    if (state[SDL_SCANCODE_D]) {
      camera->transform().position() += camera_local[0].xyz() * 0.05;
    }

    if (state[SDL_SCANCODE_UP])    x += 0.01;
    if (state[SDL_SCANCODE_DOWN])  x -= 0.01;
    if (state[SDL_SCANCODE_LEFT])  y += 0.01;
    if (state[SDL_SCANCODE_RIGHT]) y -= 0.01;
    if (state[SDL_SCANCODE_M])     z -= 0.01;
    if (state[SDL_SCANCODE_N])     z += 0.01;

    if (state[SDL_SCANCODE_P]) {
      const std::function<void(GameObject*)> traverse = [&](GameObject* object) {
        if (object->has_component<Mesh>()) {
          auto mesh = object->get_component<Mesh>();
          auto pbr_material = (PbrMaterial*)mesh->material.get();
          pbr_material->set_albedo_map({});
        }

        for (auto child : object->children()) traverse(child);
      };

      traverse(scene);
    }

    if (state[SDL_SCANCODE_O] && behemoth != nullptr) {
      scene->remove_child(behemoth);
      delete behemoth;
      behemoth = nullptr;
    }

    camera->transform().rotation().set_euler(x, y, z);

    auto euler = camera->transform().rotation().get_euler();

    renderer.render(scene);
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
