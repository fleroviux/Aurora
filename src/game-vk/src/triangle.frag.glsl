#version 450

layout(set=0, binding=0) uniform Material {
  float u_cute;
};

layout(location = 0) in vec3 v_color;

layout(location = 0) out vec4 frag_color;

void main() {
  frag_color = vec4(v_color * u_cute, 1.0);
}
