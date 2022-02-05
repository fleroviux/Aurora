#version 450

layout(set = 0, binding = 0) uniform Uniforms {
  mat4 u_transform;
};

layout(location = 0) in vec3 a_position;
//layout(location = 1) in vec3 a_normal;
//layout(location = 2) in vec2 a_uv;

void main() {
  gl_Position = u_transform * vec4(a_position, 1.0);
}
