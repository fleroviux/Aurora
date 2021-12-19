/*
 * Copyright (C) 2021 fleroviux
 */

#include <aurora/scene/geometry/geometry.hpp>
#include <aurora/scene/game_object.hpp>
#include <cstring>
#include <SDL.h>
#include <GL/glew.h>

#include <memory>
#include <optional>

using namespace Aura;

struct MeshComponent final : Component {
  MeshComponent(GameObject* owner, Geometry* geometry)
      : Component(owner)
      , geometry(geometry) {
  }

  Geometry* geometry;
};

struct OpenGLRenderer {
  struct GeometryData {
    GLuint vao;
    GLuint vbo;
    GLuint ibo;
  };

  OpenGLRenderer() {
    create_default_program();
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

layout (location = 0) in vec3 position;

uniform mat4 u_projection;
uniform mat4 u_model;

void main() {
  gl_Position = u_projection * u_model * vec4(position, 1.0);
}
    )";

    auto frag_src = R"(\
#version 330 core

layout (location = 0) out vec4 frag_color;

void main() {
  frag_color = vec4(1.0, 0.0, 0.0, 1.0);
}
    )";

    auto prog = compile_program(vert_src, frag_src);

    Assert(prog.has_value(), "AuroraRender: failed to compile default shader");

    program = prog.value();
  }

  void upload_geometry(Geometry* geometry, GeometryData& data) {
    auto& vertices = geometry->vertices;
    auto& indices = geometry->indices;

    glGenVertexArrays(1, &data.vao);
    glBindVertexArray(data.vao);

    glGenBuffers(1, &data.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, data.vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size(), vertices.data(), GL_STATIC_DRAW);

    auto& layout = vertices.layout();

    for (size_t i = 0; i < layout.attributes.size(); i++) {
      auto& attribute = layout.attributes[i];
      auto normalized = attribute.normalized ? GL_TRUE : GL_FALSE;
      GLenum type;

      switch (attribute.data_type) {
        case VertexDataType::SInt8: {
          type = GL_BYTE;
          break;
        }
        case VertexDataType::UInt8: {
          type = GL_UNSIGNED_BYTE;
          break;
        }
        case VertexDataType::SInt16: {
          type = GL_SHORT;
          break;
        }
        case VertexDataType::UInt16: {
          type = GL_UNSIGNED_SHORT;
          break;
        }
        case VertexDataType::Float16: {
          type = GL_HALF_FLOAT;
          break;
        }
        case VertexDataType::Float32: {
          type = GL_FLOAT;
          break;
        }
      }

      glVertexAttribPointer(i, attribute.components, type, normalized, layout.stride, (const void*)attribute.offset);
      glEnableVertexAttribArray(i);
    }

    glGenBuffers(1, &data.ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, data.ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size(), indices.data(), GL_STATIC_DRAW);

    geometry_data_[geometry] = data;
  }

  void upload_transform_uniforms(TransformComponent const& transform) {
    // TODO: need to fixup the depth component.
    auto projection = Matrix4::perspective(90.0, 1920/1080.0, 0.01, 100.0);

    auto u_projection = glGetUniformLocation(program, "u_projection");
    if (u_projection != -1) {
      glUniformMatrix4fv(u_projection, 1, GL_FALSE, (float*)&projection[0]);
    }

    auto u_model = glGetUniformLocation(program, "u_model");
    if (u_model != -1) {
      glUniformMatrix4fv(u_model, 1, GL_FALSE, (float*)&transform.world()[0]);
    }
  }

  void render(GameObject* scene) {
    glViewport(0, 0, 1920, 1080);
    glClearColor(0.02, 0.02, 0.02, 1.00);
    glClear(GL_COLOR_BUFFER_BIT);

    const std::function<void(GameObject*)> traverse = [&](GameObject* object) {
      auto& transform = object->transform();

      // TODO: make sure that this algorithm is correct.
      if (transform.auto_update()) {
        transform.update_local();
      }
      transform.update_world(false);

      auto mesh = object->get_component<MeshComponent>();

      if (mesh != nullptr) {
        auto geometry = mesh->geometry;

        auto data = GeometryData{};
        auto it = geometry_data_.find(geometry);

        if (it != geometry_data_.end()) {
          data = it->second;
        } else {
          upload_geometry(geometry, data);
        }

        auto& indices = geometry->indices;
        upload_transform_uniforms(transform);
        glBindVertexArray(data.vao);
        glUseProgram(program);
        switch (indices.data_type()) {
          case IndexDataType::UInt16: {
            glDrawElements(GL_TRIANGLES, indices.size() / sizeof(u16), GL_UNSIGNED_SHORT, 0);
            break;
          }
          case IndexDataType::UInt32: {
            glDrawElements(GL_TRIANGLES, indices.size() / sizeof(u32), GL_UNSIGNED_INT, 0);
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

  std::unordered_map<Geometry*, GeometryData> geometry_data_;
};

auto create_example_scene() -> GameObject* {
  auto scene = new GameObject{};

  const u16 plane_indices[] = {
    0, 1, 2,
    2, 3, 0
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

  auto plane0 = new GameObject{"Plane0"};
  auto plane1 = new GameObject{"Plane1"};
  auto plane2 = new GameObject{"Plane2"};
  plane0->add_component<MeshComponent>(geometry);
  plane1->add_component<MeshComponent>(geometry);
  plane2->add_component<MeshComponent>(geometry);
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
  auto renderer = OpenGLRenderer{};

  auto event = SDL_Event{};

  for (;;) {
    auto plane0 = scene->children()[0];
    auto plane1 = plane0->children()[0];
    auto plane2 = scene->children()[1];

    plane0->transform().rotation().z() += 0.01;
    plane1->transform().position().x() =  2.5;
    plane2->transform().position().x() = -2.5;

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
