#version 450

layout(set = 0, binding = 2) uniform sampler2D u_albedo_map; 

layout(location = 0) in vec3 v_normal;
layout(location = 1) in vec2 v_uv;

layout(location = 0) out vec4 frag_color;

void main() {
  frag_color = texture(u_albedo_map, v_uv);
}
