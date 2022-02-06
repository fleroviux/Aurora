#version 450

layout(set = 0, binding = 0) uniform Camera {
  mat4 u_projection;
  mat4 u_view;
};

layout(set = 0, binding = 1) uniform Model {
  mat4 u_model;
};

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;

layout(location = 0) out vec3 v_normal;

void main() {
  v_normal = a_normal;

  // TODO: use a u_modelview matrix to save one multiplication.
  gl_Position = u_projection * u_view * u_model * vec4(a_position, 1.0);
}
