/*
 * Copyright (C) 2022 fleroviux
 */

#pragma once

constexpr auto ssr_test_vert = R"(
  #version 450

  layout(location = 0) out vec2 v_uv;

  void main() {
    switch (gl_VertexIndex) {
      case 0: gl_Position = vec4(-3.0, -1.0, 0.0, 1.0); v_uv = vec2(-1.0, 0.0); break;
      case 1: gl_Position = vec4( 1.0, -1.0, 0.0, 1.0); v_uv = vec2( 1.0, 0.0); break;
      case 2: gl_Position = vec4( 1.0,  3.0, 0.0, 1.0); v_uv = vec2( 1.0, 2.0); break;
    }
  }
)";

constexpr auto ssr_test_frag = R"(
  #version 450

  layout(set = 0, binding = 0) uniform sampler2D u_color_map;
  layout(set = 0, binding = 1) uniform sampler2D u_depth_map;
  layout(set = 0, binding = 2) uniform sampler2D u_normal_map;

  layout(set = 0, binding = 3) uniform Camera {
    mat4 u_projection;
    mat4 u_projection_inverse;
  };

  layout(location = 0) in vec2 v_uv;

  layout(location = 0) out vec4 frag_color;

  float GetDepth(vec2 uv) {
    return texture(u_depth_map, uv).r;
  }

  vec3 GetViewPosition(vec2 uv) {
    // TODO: can we do better than this?
    float depth = GetDepth(uv);
    vec4 clip_position = vec4(vec2(v_uv.x, 1.0 - v_uv.y) * 2.0 - 1.0, depth, 1.0);
    vec4 view_position = u_projection_inverse * clip_position;

    return view_position.xyz / view_position.w;
  }

  void main() {
    const uint SAMPLE_COUNT = 128;
    const float DISTANCE = 10.0;
    //const float EPSILON = 0.01;

    vec3 view_position = GetViewPosition(v_uv);
    vec3 normal = texture(u_normal_map, v_uv).xyz * 2.0 - 1.0;
    vec3 view_direction = normalize(view_position);
    vec3 reflected = reflect(view_direction, normal);

    vec3 ray_position = view_position;
    vec3 reflection = vec3(0.0);

    for (uint i = 0; i < SAMPLE_COUNT; i++) {
      ray_position += reflected * 0.025;

      vec4 proj = u_projection * vec4(ray_position, 1.0);
      proj.xyz /= proj.w;

      vec2 uv = vec2(proj.x, -proj.y) * 0.5 + 0.5;
      float depth_expect = proj.z;
      float depth_real = GetDepth(uv);

      if (depth_real <= depth_expect) {
        reflection = texture(u_color_map, vec2(uv.x, uv.y)).rgb;
        break;
      }
    }

    const float F0 = 0.04;

    float n_dot_v = max(0.0, dot(normal, -view_direction));
    float fresnel = F0 + (1.0 - F0) * pow(1.0 - n_dot_v, 5.0);
    reflection *= fresnel;

    frag_color = texture(u_color_map, v_uv) + vec4(reflection, 0.0);
  }
)";
