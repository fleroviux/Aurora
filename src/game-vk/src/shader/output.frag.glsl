#version 450

layout(set = 0, binding = 1) uniform sampler2D u_diffuse_map;

layout(location = 0) in vec2 v_uv;

layout(location = 0) out vec4 frag_color;

void main() {
  //frag_color = vec4(v_color, 1.0);
  frag_color = texture(u_diffuse_map, v_uv + vec2(0.005 * sin(v_uv.y * 50.0), 0.0));
}
