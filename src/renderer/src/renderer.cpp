/*
 * Copyright (C) 2021 fleroviux
 */

#include <aurora/renderer/renderer.hpp>

#include "pbr.glsl.hpp"

namespace Aura {

OpenGLRenderer::OpenGLRenderer() {
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);
  glEnable(GL_TEXTURE_2D);

  create_default_program();

  default_texture_ = Texture::load("cirno.jpg");
}

OpenGLRenderer::~OpenGLRenderer() {
}

void OpenGLRenderer::render(GameObject* scene) {
  // TODO: validate that the scene component exists and the camera is valid.
  auto camera = scene->get_component<Scene>()->camera;

  glViewport(0, 0, 2560, 1440);
  glClearColor(0.02, 0.02, 1.0, 1.00);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  const std::function<void(GameObject*)> traverse = [&](GameObject* object) {
    auto& transform = object->transform();

    // TODO: this algorithm breaks with the camera at least.
    if (transform.auto_update()) {
      transform.update_local();
    }
    transform.update_world(false);

    auto mesh = object->get_component<Mesh>();

    if (mesh != nullptr) {
      auto geometry = mesh->geometry.get();
      auto material = mesh->material.get();

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
      
      uniform_block.get<float>("metalness") = material->metalness;
      uniform_block.get<float>("roughness") = material->roughness;

      auto block_index = glGetUniformBlockIndex(program, "material");
      glBindBuffer(GL_UNIFORM_BUFFER, ubo);
      glBufferData(GL_UNIFORM_BUFFER, uniform_block.size(), uniform_block.data(), GL_DYNAMIC_DRAW);
      glBindBufferBase(GL_UNIFORM_BUFFER, 0, ubo);
      glUniformBlockBinding(program, block_index, 0);

      // TODO: rewrite this... it is terrifying.
      {
        // TODO: rename uniform to u_albedo_map or rename Material::albedo
        auto u_diffuse_map = glGetUniformLocation(program, "u_diffuse_map");
        auto u_metalness_map = glGetUniformLocation(program, "u_metalness_map");
        auto u_roughness_map = glGetUniformLocation(program, "u_roughness_map");
        auto u_normal_map = glGetUniformLocation(program, "u_normal_map");

        if (u_diffuse_map != -1) {
          glUniform1i(u_diffuse_map, 0);
        }

        if (u_metalness_map != -1) {
          glUniform1i(u_metalness_map, 1);
        }

        if (u_roughness_map != -1) {
          glUniform1i(u_roughness_map, 2);
        }

        if (u_roughness_map != -1) {
          glUniform1i(u_normal_map, 3);
        }

        if (material->albedo) {
          bind_texture(GL_TEXTURE0, material->albedo.get());
        } else {
          bind_texture(GL_TEXTURE0, default_texture_.get());
        }

        if (material->metalness_map) {
          bind_texture(GL_TEXTURE1, material->metalness_map.get());
        } else {
          bind_texture(GL_TEXTURE1, default_texture_.get());
        }

        if (material->roughness_map) {
          bind_texture(GL_TEXTURE2, material->roughness_map.get());
        } else {
          bind_texture(GL_TEXTURE2, default_texture_.get());
        }

        if (material->normal_map) {
          bind_texture(GL_TEXTURE3, material->normal_map.get());
        } else {
          bind_texture(GL_TEXTURE3, default_texture_.get());
        }
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

auto OpenGLRenderer::upload_texture(Texture* texture) -> GLuint {
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

void OpenGLRenderer::upload_geometry(Geometry* geometry, GeometryData& data) {
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

void OpenGLRenderer::bind_texture(GLenum slot, Texture* texture) {
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

void OpenGLRenderer::upload_transform_uniforms(Transform const& transform, GameObject* camera) {
  // TODO: validate that the camera component exists.
  auto camera_component = camera->get_component<Camera>();
  
  auto projection = Matrix4::perspective_gl(
    camera_component->field_of_view,
    camera_component->aspect_ratio,
    camera_component->near,
    camera_component->far
  );
  
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

void OpenGLRenderer::create_default_program() {
  auto prog = compile_program(pbr_vert, pbr_frag);

  Assert(prog.has_value(), "AuroraRender: failed to compile default shader");

  program = prog.value();

  auto layout = UniformBlockLayout{};
  layout.add<float>("metalness");
  layout.add<float>("roughness");
  uniform_block = UniformBlock{layout};

  glGenBuffers(1, &ubo);
}

auto OpenGLRenderer::compile_shader(
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

auto OpenGLRenderer::compile_program(
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

auto OpenGLRenderer::get_gl_attribute_type(VertexDataType data_type) -> GLenum {
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

} // namespace Aura

