/*
 * Copyright (C) 2021 fleroviux
 */

#include <aurora/scene/geometry/geometry.hpp>
#include <aurora/scene/game_object.hpp>
#include <cstring>
#include <SDL.h>
#include <GL/glew.h>

#include "gltf_loader.hpp"
#include "texture.hpp"

#include <memory>
#include <optional>
#include <stb_image.h>

using namespace Aura;

struct OpenGLRenderer {
  struct GeometryData {
    GLuint vao;
    GLuint ibo;
    std::vector<GLuint> vbos;
  };

  OpenGLRenderer() {
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_TEXTURE_2D);

    create_default_program();

    default_texture_ = Texture::load("cirno.jpg");

    // Set texture uniform 
    auto u_diffuse_map = glGetUniformLocation(program, "u_diffuse_map");
    if (u_diffuse_map != -1) {
      glUniform1i(u_diffuse_map, 0);
    }
  }

 ~OpenGLRenderer() {
  }

  auto compile_shader(
    GLenum type,
    char const* source
  ) -> std::optional<GLuint> {
    char const* source_array[] = { source };

    auto shader = glCreateShader(type);

    glShaderSource(shader, 1, source_array, nullptr);
    glCompileShader(shader);

    GLint compiled = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (compiled == GL_FALSE) {
      GLint max_length = 0;
      glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &max_length);

      auto error_log = std::make_unique<GLchar[]>(max_length);
      glGetShaderInfoLog(shader, max_length, &max_length, error_log.get());
      Log<Error>("OpenGLRenderer: failed to compile shader:\n{}", error_log.get());
      return {};
    }

    return shader;
  }

  auto compile_program(
    char const* vert_src,
    char const* frag_src
  ) -> std::optional<GLuint> {
    auto vert = compile_shader(GL_VERTEX_SHADER, vert_src);
    auto frag = compile_shader(GL_FRAGMENT_SHADER, frag_src);

    if (vert.has_value() && frag.has_value()) {
      auto prog = glCreateProgram();

      glAttachShader(prog, vert.value());
      glAttachShader(prog, frag.value());
      glLinkProgram(prog);
      glDeleteShader(vert.value());
      glDeleteShader(frag.value());
      return prog;
    }

    return {};
  }

  void create_default_program() {
    auto vert_src = R"(\
#version 330 core

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec3 a_normal;
layout (location = 2) in vec2 a_uv;
layout (location = 3) in vec3 a_color;

// TODO: pass a unified modelview matrix.
uniform mat4 u_projection;
uniform mat4 u_model;
uniform mat4 u_view;

out vec3 v_color;
out vec2 v_uv;
out vec3 v_normal;

void main() {
  v_color = a_color;
  v_uv = a_uv;
  v_normal = a_normal;
  gl_Position = u_projection * u_view * u_model * vec4(a_position, 1.0);
}
    )";

    auto frag_src = R"(\
#version 330 core

layout (location = 0) out vec4 frag_color;

in vec3 v_color;
in vec2 v_uv;
in vec3 v_normal;

uniform sampler2D u_diffuse_map;

void main() {
  vec4 diffuse = texture(u_diffuse_map, v_uv);
  if (diffuse.a < 0.5) {
    discard;
  }
  frag_color = vec4(diffuse.rgb, 1.0);
}
    )";

    auto prog = compile_program(vert_src, frag_src);

    Assert(prog.has_value(), "AuroraRender: failed to compile default shader");

    program = prog.value();
  }

  auto upload_texture(Texture* texture) -> GLuint {
    GLuint id;

    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST_MIPMAP_LINEAR);

    glTexImage2D(
      GL_TEXTURE_2D,
      0,
      GL_RGBA,
      texture->width(),
      texture->height(),
      0,
      GL_RGBA,
      GL_UNSIGNED_BYTE,
      texture->data()
    );

    glGenerateMipmap(GL_TEXTURE_2D);

    texture_data_[texture] = id;
    return id;
  }

  static auto get_gl_attribute_type(VertexDataType data_type) -> GLenum {
    switch (data_type) {
      case VertexDataType::SInt8: return GL_BYTE;
      case VertexDataType::UInt8: return GL_UNSIGNED_BYTE;
      case VertexDataType::SInt16: return GL_SHORT;
      case VertexDataType::UInt16: return GL_UNSIGNED_SHORT;
      case VertexDataType::Float16: return GL_HALF_FLOAT;
      case VertexDataType::Float32: return GL_FLOAT;
    }

    Assert(false, "AuroraRender: unsupported vertex data type: {}", (int)data_type);
  }

  void upload_geometry(Geometry* geometry, GeometryData& data) {
    auto& index_buffer = geometry->index_buffer;

    glGenVertexArrays(1, &data.vao);
    glBindVertexArray(data.vao);

    glGenBuffers(1, &data.ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, data.ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_buffer.size(), index_buffer.data(), GL_STATIC_DRAW);

    for (auto& buffer : geometry->buffers) {
      GLuint vbo;

      glGenBuffers(1, &vbo);
      glBindBuffer(GL_ARRAY_BUFFER, vbo);
      glBufferData(GL_ARRAY_BUFFER, buffer.size(), buffer.data(), GL_STATIC_DRAW);

      auto& layout = buffer.layout();

      for (size_t i = 0; i < layout.attributes.size(); i++) {
        auto& attribute = layout.attributes[i];
        auto normalized = attribute.normalized ? GL_TRUE : GL_FALSE;
        auto type = get_gl_attribute_type(attribute.data_type);

        glVertexAttribPointer(
          attribute.index,
          attribute.components,
          type,
          normalized,
          layout.stride,
          (const void*)attribute.offset);

        glEnableVertexAttribArray(attribute.index);
      }

      data.vbos.push_back(vbo);
    }

    geometry_data_[geometry] = data;
  }

  void bind_texture(GLenum slot, Texture* texture) {
    auto match = texture_data_.find(texture);
    auto id = GLuint{};

    if (match == texture_data_.end()) {
      id = upload_texture(texture);
    } else {
      id = match->second;
    }

    glActiveTexture(slot);
    glBindTexture(GL_TEXTURE_2D, id);
  }

  void upload_transform_uniforms(TransformComponent const& transform, GameObject* camera) {
    // TODO: need to fixup the depth component.
    auto projection = Matrix4::perspective(45.0, 1600/900.0, 0.01, 2000.0);
    auto view = camera->transform().world().inverse();

    auto u_projection = glGetUniformLocation(program, "u_projection");
    if (u_projection != -1) {
      glUniformMatrix4fv(u_projection, 1, GL_FALSE, (float*)&projection[0]);
    }

    auto u_model = glGetUniformLocation(program, "u_model");
    if (u_model != -1) {
      glUniformMatrix4fv(u_model, 1, GL_FALSE, (float*)&transform.world()[0]);
    }

    auto u_view = glGetUniformLocation(program, "u_view");
    if (u_view != -1) {
      glUniformMatrix4fv(u_view, 1, GL_FALSE, (float*)&view[0]); 
    }
  }

  void render(GameObject* scene, GameObject* camera) {
    glViewport(0, 0, 1600, 900);
    glClearColor(0.02, 0.02, 0.02, 1.00);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    const std::function<void(GameObject*)> traverse = [&](GameObject* object) {
      auto& transform = object->transform();

      // TODO: this algorithm breaks with the camera at least.
      if (transform.auto_update()) {
        transform.update_local();
      }
      transform.update_world(false);

      auto mesh = object->get_component<MeshComponent>();

      if (mesh != nullptr) {
        auto geometry = mesh->geometry;
        auto material = mesh->material;

        auto data = GeometryData{};
        auto it = geometry_data_.find(geometry);

        if (it != geometry_data_.end()) {
          data = it->second;
        } else {
          upload_geometry(geometry, data);
        }

        // TODO: check GL_INVALID_OPERATION on first draw?
        auto& index_buffer = geometry->index_buffer;
        upload_transform_uniforms(transform, camera);
        glBindVertexArray(data.vao);
        glUseProgram(program);
        if (material->albedo) {
          bind_texture(GL_TEXTURE0, material->albedo.get());
        } else {
          bind_texture(GL_TEXTURE0, default_texture_.get());
        }
        switch (index_buffer.data_type()) {
          case IndexDataType::UInt16: {
            glDrawElements(GL_TRIANGLES, index_buffer.size() / sizeof(u16), GL_UNSIGNED_SHORT, 0);
            break;
          }
          case IndexDataType::UInt32: {
            glDrawElements(GL_TRIANGLES, index_buffer.size() / sizeof(u32), GL_UNSIGNED_INT, 0);
            break;
          }
        }
      }

      for (auto child : object->children()) traverse(child);
    };

    traverse(scene);
  }

private:
  GLuint program;

  std::unique_ptr<Texture> default_texture_;

  std::unordered_map<Texture*, GLuint> texture_data_;
  std::unordered_map<Geometry*, GeometryData> geometry_data_;
};

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

  auto geometry = new Geometry{index_buffer};
  geometry->buffers.push_back(std::move(vertex_buffer));

  auto material = new Material{};

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
  auto cyoob = gltf_loader.parse("DamagedHelmet/DamagedHelmet.gltf");
  //cyoob->transform().scale() = Vector3{0.05, 0.05, 0.05};
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
