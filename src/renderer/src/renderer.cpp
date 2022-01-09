/*
 * Copyright (C) 2021 fleroviux
 */

#include <aurora/renderer/renderer.hpp>

//#include "pbr.glsl.hpp"

namespace Aura {

OpenGLRenderer::OpenGLRenderer() {
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);
  glEnable(GL_TEXTURE_2D);
  glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &gl_max_anisotropy);
  glEnable(GL_CULL_FACE);

  auto layout_camera = UniformBlockLayout{};
  layout_camera.add<Matrix4>("view");
  layout_camera.add<Matrix4>("projection");
  uniform_camera = UniformBlock{layout_camera};
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
    if (object->visible()) {
      auto& transform = object->transform();

      // TODO: this algorithm breaks with the camera at least.
      if (transform.auto_update()) {
        transform.update_local();
      }
      transform.update_world(false);

      auto mesh = object->get_component<Mesh>();

      if (mesh != nullptr && mesh->visible) {
        auto geometry = mesh->geometry.get();
        auto material = mesh->material.get();

        bind_material(material, object);
        draw_geometry(geometry);
      }

      for (auto child : object->children()) traverse(child);
    }
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
  uniform_camera.needs_update() = true;
}

void OpenGLRenderer::bind_uniform_block(
  UniformBlock& uniform_block,
  GLuint program,
  size_t binding
) {
  auto match = uniform_block_cache_.find(&uniform_block);
  auto ubo = GLuint{};
  bool force_update = false;

  if (match == uniform_block_cache_.end()) {
    glGenBuffers(1, &ubo);
    uniform_block_cache_[&uniform_block] = ubo;
    force_update = true;

    uniform_block.add_release_callback([this, uniform_block = &uniform_block, ubo]() {
      glDeleteBuffers(1, &ubo);
      uniform_block_cache_.erase(uniform_block);
    });
  } else {
    ubo = match->second;
  }

  if (uniform_block.needs_update() || force_update) {
    glBindBuffer(GL_UNIFORM_BUFFER, ubo);
    glBufferData(GL_UNIFORM_BUFFER, uniform_block.size(), uniform_block.data(), GL_DYNAMIC_DRAW);
    uniform_block.needs_update() = false;
  }

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

auto OpenGLRenderer::get_program(Material* material) -> GLuint {
  auto compile_options = material->get_compile_options();
  auto key = ProgramCacheKey{
    std::type_index{typeid(material)}, compile_options};
  auto match = program_cache_.find(key);

  if (match != program_cache_.end()) {
    return match->second;
  }

  auto prologue = std::string{"#version 420 core\n\n"};
  auto& compile_option_names = material->get_compile_option_names();

  for (size_t i = 0; i< compile_option_names.size(); i++) {
    if (compile_options & (1 << i)) {
      prologue += fmt::format("#define {}\n", compile_option_names[i]);
    }
  }

  auto vert_shader = fmt::format("{}\n{}", prologue, material->get_vert_shader());
  auto frag_shader = fmt::format("{}\n{}", prologue, material->get_frag_shader());

  auto program = compile_program(vert_shader.c_str(), frag_shader.c_str());

  Assert(program.has_value(), "AuroraRender: failed to compile program");

  program_cache_[key] = program.value();
  return program.value();
}

void OpenGLRenderer::bind_material(Material* material, GameObject* object) {
  auto program = get_program(material);
  auto& uniforms = material->get_uniforms();

  glUseProgram(program);

  uniforms.get<Matrix4>("model") = object->transform().world();
  uniforms.needs_update() = true;

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

  glCullFace(GL_BACK);
}

void OpenGLRenderer::draw_geometry(Geometry* geometry) {
  auto match = geometry_cache_.find(geometry);

  if (match == geometry_cache_.end()) {
    create_geometry(geometry);    
    match = geometry_cache_.find(geometry);
  }

  auto& data = match->second;
  update_geometry_ibo(geometry, data, false);
  update_geometry_vbo(geometry, data, false);

  auto& index_buffer = geometry->index_buffer;
  
  glBindVertexArray(data.vao);

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

void OpenGLRenderer::create_geometry(Geometry* geometry) {
  auto& data = geometry_cache_[geometry];

  glGenVertexArrays(1, &data.vao);
  glBindVertexArray(data.vao);
  glGenBuffers(1, &data.ibo);

  for (auto& buffer : geometry->buffers) {
    auto vbo = GLuint{};
    auto& layout = buffer.layout();
    
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

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

  update_geometry_ibo(geometry, data, true);
  update_geometry_vbo(geometry, data, true);

  geometry->add_release_callback([this, geometry, data]() {
    glDeleteVertexArrays(1, &data.vao);
    glDeleteBuffers(1, &data.ibo);
    glDeleteBuffers(data.vbos.size(), data.vbos.data());
    geometry_cache_.erase(geometry);
  });
}

void OpenGLRenderer::update_geometry_ibo(
  Geometry* geometry,
  GeometryCacheEntry& data,
  bool force_update
) {
  auto& index_buffer = geometry->index_buffer;

  if (index_buffer.needs_update() || force_update) {
    glBindVertexArray(data.vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, data.ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_buffer.size(), index_buffer.data(), GL_STATIC_DRAW);
    index_buffer.needs_update() = false;
  }
}

void OpenGLRenderer::update_geometry_vbo(
  Geometry* geometry,
  GeometryCacheEntry& data,
  bool force_update
) {
  auto buffer_count = geometry->buffers.size();

  for (size_t i = 0; i < buffer_count; i++) {
    auto& buffer = geometry->buffers[i];

    if (buffer.needs_update() || force_update) {
      glBindVertexArray(data.vao);
      glBindBuffer(GL_ARRAY_BUFFER, data.vbos[i]);
      glBufferData(GL_ARRAY_BUFFER, buffer.size(), buffer.data(), GL_STATIC_DRAW);
      buffer.needs_update() = false;
    }
  }
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

