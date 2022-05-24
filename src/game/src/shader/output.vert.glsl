#version 450

layout(location = 0) out vec2 v_uv;

void main() {
  switch (gl_VertexIndex) {
    case 0: gl_Position = vec4(-3.0, -1.0, 0.0, 1.0); v_uv = vec2(-1.0, 0.0); break;
    case 1: gl_Position = vec4( 1.0, -1.0, 0.0, 1.0); v_uv = vec2( 1.0, 0.0); break;
    case 2: gl_Position = vec4( 1.0,  3.0, 0.0, 1.0); v_uv = vec2( 1.0, 2.0); break;
  }
}
