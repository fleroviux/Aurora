/*
 * Copyright (C) 2021 fleroviux
 */

#include <aurora/renderer/renderer.hpp>

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

void OpenGLRenderer::render(GameObject* scene, GameObject* camera) {
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
      
      // TODO: rewrite this... it is terrifying.
      {
        // TODO: rename uniform to u_albedo_map or rename Material::albedo
        auto u_diffuse_map = glGetUniformLocation(program, "u_diffuse_map");
        auto u_metalness_map = glGetUniformLocation(program, "u_metalness_map");
        auto u_roughness_map = glGetUniformLocation(program, "u_roughness_map");
        auto u_normal_map = glGetUniformLocation(program, "u_normal_map");
        auto u_metalness = glGetUniformLocation(program, "u_metalness");
        auto u_roughness = glGetUniformLocation(program, "u_roughness");

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

        if (u_metalness != -1) {
          glUniform1f(u_metalness, material->metalness);
        }

        if (u_roughness != -1) {
          glUniform1f(u_roughness, material->roughness);
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

void OpenGLRenderer::upload_transform_uniforms(TransformComponent const& transform, GameObject* camera) {
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

void OpenGLRenderer::create_default_program() {
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

out vec3 v_world_position;
out vec3 v_world_normal;
out vec3 v_view_position;
out vec3 v_color;
out vec2 v_uv;
out vec3 v_normal;

void main() {
  vec4 world_position = u_model * vec4(a_position, 1.0);
  vec4 view_position = u_view * world_position;

  // TODO: handle non-uniform scale in normal matrix.
  v_world_position = world_position.xyz;
  v_view_position = view_position.xyz;
  v_world_normal = (u_model * vec4(a_normal, 0.0)).xyz;

  v_color = a_color;
  v_uv = a_uv;
  v_normal = a_normal;
  gl_Position = u_projection * view_position;
}
  )";

  auto frag_src = R"(\
#version 330 core

// PBR library begin

#define PI 3.141592

// Normal-distribution function
float ThrowbridgeReitzNDF(float n_dot_h, float roughness) {
  float a2 = roughness * roughness;
  float tmp = n_dot_h * n_dot_h * (a2 - 1.0) + 1.0;

  return a2 / (tmp * tmp);
}

// Geometric Occlusion
float GeometrySchlickGGX(float n_dot_v, float k) {
  return n_dot_v / (n_dot_v * (1.0 - k) + k);
}
float GeometrySmith(float n_dot_v, float n_dot_l, float roughness) {
  // TODO: LearnOpenGL suggests a different calculation for IBL. Why?
  float k = (roughness + 1.0) * (roughness + 1.0) / 8.0;

  return GeometrySchlickGGX(n_dot_v, k) * GeometrySchlickGGX(n_dot_l, k);
}

// Fresnel
// TODO: what is the correct angle to use for fresnel?
vec3 FresnelSchlick(vec3 f0, float n_dot_v) {
  return f0 + (1.0 - f0) * pow(1.0 - n_dot_v, 5.0);
}

// BRDF = ((1 - F) * albedo/PI + F * NG/(4 * n_dot_v * n_dot_l)) * n_dot_l * light_intensity

struct Geometry {
  vec3 position;
  vec3 normal;

  vec3 albedo;
  float metalness;
  float roughness;
};

struct Light {
  vec4 position;
  vec3 color;
};

vec3 ShadeDirectLight(in Geometry geometry, in Light light, vec3 view_dir) {
  // w = 0: directional light
  // w = 1: point/omnidirectional light
  vec3 light_dir = light.position.xyz - geometry.position * light.position.w;
  float distance_sqr = dot(light_dir, light_dir);
  float attenuation = 1.0 / (1.0 + distance_sqr);
  light_dir *= inversesqrt(distance_sqr);

  vec3 halfway_dir = normalize(light_dir + view_dir);

  // TODO: check which dot-products need clamping and which don't.
  // TODO: can we calculate fresnel accurately without calculating h_dot_v?
  float n_dot_v = max(0.0, dot(geometry.normal, view_dir));
  float n_dot_h = max(0.0, dot(geometry.normal, halfway_dir));
  float n_dot_l = max(0.0, dot(geometry.normal, light_dir));

  vec3 f0 = mix(vec3(0.04), geometry.albedo, geometry.metalness);
  vec3 f = FresnelSchlick(f0, n_dot_v);
  float n = ThrowbridgeReitzNDF(n_dot_h, geometry.roughness);
  float g = GeometrySmith(n_dot_v, n_dot_l, geometry.roughness);

  // TODO: make sure that the terminology is used correctly;
  vec3 irradiance = n_dot_l * light.color * attenuation;

  // TODO: how to properly handle the possible division-by-zero in the specular term?
  vec3 diffuse = (1.0 - f) * geometry.albedo * irradiance * (1.0 - geometry.metalness);
  vec3 specular = (f * n * g) / max((4.0 * n_dot_l * n_dot_v), 0.001) * irradiance * PI;

  return diffuse + specular;
}

// PBR library end

layout (location = 0) out vec4 frag_color;

in vec3 v_world_position;
in vec3 v_world_normal;
in vec3 v_view_position;
in vec3 v_color;
in vec2 v_uv;
in vec3 v_normal;

uniform mat4 u_view;
uniform float u_metalness;
uniform float u_roughness;
uniform sampler2D u_diffuse_map;
uniform sampler2D u_metalness_map;
uniform sampler2D u_roughness_map;
uniform sampler2D u_normal_map;

vec3 sRGBToLinear(vec3 color) {
  return pow(color, vec3(2.2));
}

vec3 LinearTosRGB(vec3 color) {
  return pow(color, vec3(1.0/2.2));
}

// Source: https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
vec3 ACESFilm(vec3 x) {
  float a = 2.51f;
  float b = 0.03f;
  float c = 2.43f;
  float d = 0.59f;
  float e = 0.14f;

  return clamp((x*(a*x+b))/(x*(c*x+d)+e), vec3(0.0), vec3(1.0));
}

vec3 PerturbNormal() {
  vec3 tbn_normal = texture(u_normal_map, v_uv).xyz * 2.0 - 1.0;

  vec2 dUVdx = dFdx(v_uv);
  vec2 dUVdy = dFdy(v_uv);

  vec3 dPdx = dFdx(v_world_position);
  vec3 dPdy = dFdy(v_world_position);

  /*
   * dPdx = T * dUVdx.x + B * dUVdx.y
   * dPdy = T * dUVdy.x + B * dUVdy.y
   *
   * | dUVdx.x  dUVdx.y | * | T.x  T.y  T.z | = | dPdx.x  dPdx.y dPdx.z |
   * | dUVdy.x  dUVdy.y |   | B.x  B.y  B.z |   | dPdy.x  dPdy.y dPdy.z |
   *
   * Take 2x2 matrix inverse of dUVdx/dy matrix to solve for T and B.
   *
   * det = dUVdx.x * dUVdy.y - dUVdy.x * dUVdx.y
   *
   * inverse = 1/det * |  dUVdy.y   -dUVdx.y |
   *                   | -dUVdy.x    dUVdx.x |
   */

  float det = dUVdx.x * dUVdy.y - dUVdy.x * dUVdx.y;

  // TODO: handle backfaces?
  float flip = sign(det);

  vec3 T = normalize( dUVdy.y * dPdx - dUVdx.y * dPdy) * flip;
  vec3 B = normalize(-dUVdy.x * dPdx + dUVdx.x * dPdy) * flip;
  vec3 N = normalize(v_world_normal);

  if (det == 0.0) {
    // There is no solution because the UVs are degenerate.
    return N;
  }

  return mat3(T, B, N) * tbn_normal;
}

void main() {
  vec4 diffuse = texture(u_diffuse_map, v_uv);
  if (diffuse.a < 0.5) {
    discard;
  }

  diffuse.rgb = sRGBToLinear(diffuse.rgb);

  float metalness = u_metalness * texture(u_metalness_map, v_uv).b;
  float roughness = u_roughness * texture(u_roughness_map, v_uv).g;

  vec3 view_dir = -normalize(v_view_position);
  view_dir = (vec4(view_dir, 0.0) * u_view).xyz;

  Geometry geometry;
  geometry.position = v_world_position;
  geometry.normal = PerturbNormal();
  geometry.albedo = diffuse.rgb;
  geometry.metalness = metalness;
  geometry.roughness = max(roughness, 0.001);

  vec3 result = vec3(0.0);

  Light my_light;
  my_light.position = vec4(2.0, 2.0, 2.0, 0.0);
  my_light.color = vec3(10.0);
  result += ShadeDirectLight(geometry, my_light, view_dir);

  //my_light.position = vec4(4.0, 10.0, -3.0, 1.0);
  //my_light.color = vec3(0.0, 10.0, 8.0);
  //result += ShadeDirectLight(geometry, my_light, view_dir);

  result = ACESFilm(result);
  result = LinearTosRGB(result);

  frag_color = vec4(result, 1.0);
}
  )";

  auto prog = compile_program(vert_src, frag_src);

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

