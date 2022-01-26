#version 450

layout(set = 0, binding = 0) uniform Uniforms {
  mat4 u_transform;
};

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_color;

layout(location = 0) out vec3 v_color;

void main() {
  v_color = a_color;
  gl_Position = u_transform * vec4(a_position, 1.0);
}
