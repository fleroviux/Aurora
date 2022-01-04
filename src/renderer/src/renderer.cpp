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
  glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &gl_max_anisotropy);

  auto layout_camera = UniformBlockLayout{};
  layout_camera.add<Matrix4>("view");
  layout_camera.add<Matrix4>("projection");
  uniform_camera = UniformBlock{layout_camera};

  create_default_program();
}

OpenGLRenderer::~OpenGLRenderer() {
}

void OpenGLRenderer::render(GameObject* scene) {
  glViewport(0, 0, 2560, 1440);
  glClearColor(0.02, 0.02, 1.0, 1.00);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // TODO: validate that the scene component exists and the camera is valid.
  update_camera_transform(scene->get_component<Scene>()->camera);

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

      bind_material(material, object);
      draw_geometry(geometry);
    }

    for (auto child : object->children()) traverse(child);
  };

  traverse(scene);
}

void OpenGLRenderer::update_camera_transform(GameObject* camera) {
  // TODO: validate that the camera component exists.
  auto camera_component = camera->get_component<Camera>();
  
  auto view = camera->transform().world().inverse();

  auto projection = Matrix4::perspective_gl(
    camera_component->field_of_view,
    camera_component->aspect_ratio,
    camera_component->near,
    camera_component->far
  );

  uniform_camera.get<Matrix4>("view") = view;
  uniform_camera.get<Matrix4>("projection") = projection;
}

void OpenGLRenderer::upload_geometry(Geometry const* geometry, GeometryCacheEntry& data) {
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

  geometry_cache_[geometry] = data;
}

void OpenGLRenderer::bind_uniform_block(
  UniformBlock const& uniform_block,
  GLuint program,
  size_t binding
) {
  auto match = uniform_block_cache_.find(&uniform_block);
  auto ubo = GLuint{};

  if (match == uniform_block_cache_.end()) {
    glGenBuffers(1, &ubo);
    uniform_block_cache_[&uniform_block] = ubo;
  } else {
    ubo = match->second;
  }

  glBindBuffer(GL_UNIFORM_BUFFER, ubo);
  glBufferData(GL_UNIFORM_BUFFER, uniform_block.size(), uniform_block.data(), GL_DYNAMIC_DRAW);
  glBindBufferBase(GL_UNIFORM_BUFFER, binding, ubo);
  glUniformBlockBinding(program, binding, binding);
}

void OpenGLRenderer::bind_texture(Texture* texture, GLenum slot) {
  auto match = texture_cache_.find(texture);
  auto id = GLuint{};
  bool force_update = false;

  if (match == texture_cache_.end()) {
    glGenTextures(1, &id);
    texture_cache_[texture] = id;
    force_update = true;

    texture->add_release_callback([=]() {
      texture_cache_.erase(texture);
      glDeleteTextures(1, &id);
    });
  } else {
    id = match->second;
  }

  if (texture->needs_update() || force_update) {
    // TODO: decouple texture configuration from texture data?
    glBindTexture(GL_TEXTURE_2D, id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, gl_max_anisotropy);

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

    texture->needs_update() = false;
  }

  glActiveTexture(slot);
  glBindTexture(GL_TEXTURE_2D, id);
}

void OpenGLRenderer::bind_material(Material* material, GameObject* object) {
  glUseProgram(program);

  auto& uniforms = material->get_uniforms();

  uniforms.get<Matrix4>("model") = object->transform().world();

  bind_uniform_block(uniform_camera, program, 0);
  bind_uniform_block(uniforms, program, 1);

  auto texture_slots = material->get_texture_slots();
  auto texture_slot_count = texture_slots.size();

  for (size_t slot = 0; slot < texture_slot_count; slot++) {
    auto& texture = texture_slots[slot];

    if (texture) {
      bind_texture(texture.get(), GL_TEXTURE0 + slot);
    }
  }
}

void OpenGLRenderer::draw_geometry(Geometry const* geometry) {
  auto data = GeometryCacheEntry{};
  auto it = geometry_cache_.find(geometry);

  if (it != geometry_cache_.end()) {
    data = it->second;
  } else {
    upload_geometry(geometry, data);
  }

  auto& index_buffer = geometry->index_buffer;

  glBindVertexArray(data.vao);

  // TODO: check GL_INVALID_OPERATION on first draw?
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

void OpenGLRenderer::create_default_program() {
  auto prog = compile_program(pbr_vert, pbr_frag);

  Assert(prog.has_value(), "AuroraRender: failed to compile default shader");

  program = prog.value();
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

