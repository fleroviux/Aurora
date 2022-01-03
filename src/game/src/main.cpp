/*
 * Copyright (C) 2021 fleroviux
 */

#include <aurora/math/rotation.hpp>
#include <aurora/renderer/renderer.hpp>
#include <aurora/renderer/uniform_block_layout.hpp>
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
  plane0->add_component<Mesh>(geometry, material);
  plane1->add_component<Mesh>(geometry, material);
  plane2->add_component<Mesh>(geometry, material);
  plane0->add_child(plane1);
  scene->add_child(plane0);
  scene->add_child(plane2);
  return scene;
}

void test_ubo() {
  auto layout_a = UniformBlockLayout{};
  layout_a.add<float>("roughness");
  layout_a.add<float>("metalness");
  layout_a.add<Vector3>("albedo");
  layout_a.add<Vector2>("uv_offset");
  layout_a.add<Matrix4>("some_matrix");
  layout_a.add<float>("my_array", 6);

  fmt::print("{}", layout_a.to_string());

  auto block = UniformBlock{layout_a};

  block.get<float>("roughness") = 0.1337;
  block.get<float>("metalness") = 0.6969;
  block.get<Matrix4>("some_matrix") = Matrix4::scale(0.1, 0.2, 0.3);

  fmt::print("{}\n", block.get<float>("metalness"));
  fmt::print("{}\n", block.get<float>("roughness"));
  for (int r = 0; r < 4; r++) {
    auto& m = block.get<Matrix4>("some_matrix");
    fmt::print("{:.2f} {:.2f} {:.2f} {:.2f}\n",
      m[0][r], m[1][r], m[2][r], m[3][r]);
  }
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
  camera->add_component<Camera>();
  scene->add_component<Scene>(camera);
  scene->add_child(camera);

  auto behemoth = GLTFLoader{}.parse("DamagedHelmet/DamagedHelmet.gltf");
  scene->add_child(behemoth);

  test_ubo();

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

    camera->transform().rotation().set_euler(x, y, z);

    auto euler = camera->transform().rotation().get_euler();

    //Log<Info>("camera rotation: {:.4f} {:.4f} {:.4f}",
    //  euler.x(), euler.y(), euler.z());

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
