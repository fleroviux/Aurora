
// Copyright (C) 2022 fleroviux. All rights reserved.

#pragma once

constexpr auto raytrace_vert = R"(
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

constexpr auto raytrace_frag = R"(
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

  vec3 GetNormal(vec2 uv) {
    return texture(u_normal_map, uv).xyz * 2.0 - 1.0;
  }

  void main() {
    const uint SAMPLE_COUNT = 128;
    const float DISTANCE = 10.0;
    const float THRESHOLD = 0.05;

    vec3 view_position = GetViewPosition(v_uv);
    vec3 normal = GetNormal(v_uv);
    vec3 view_direction = normalize(view_position);
    vec3 reflected = reflect(view_direction, normal);

    vec3 ray_position = view_position + reflected * 0.05;
    vec3 reflection = vec3(0.0);

    for (uint i = 0; i < SAMPLE_COUNT; i++) {
      vec4 proj = u_projection * vec4(ray_position, 1.0);
      proj.xyz /= proj.w;

      vec2 uv = vec2(proj.x, -proj.y) * 0.5 + 0.5;

      // TODO: can this be better solved using clamp-to-edge wrapping?
      if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) {
        break;
      }

      //float depth_expect = proj.z;
      //float depth_real = GetDepth(uv);

      // TODO: simplify this calculation?
      float z = GetViewPosition(uv).z;
      float z_diff = z - ray_position.z;

      if (z_diff >= 0.0 && z_diff < THRESHOLD) {
        // avoid hitting backfaces
        if (dot(reflected, GetNormal(uv)) < 0.0) {
          reflection = texture(u_color_map, vec2(uv.x, uv.y)).rgb;
        }
        break;
      }

      ray_position += reflected * 0.025;
    }

    const float F0 = 0.04;

    float n_dot_v = max(0.0, dot(normal, -view_direction));
    float fresnel = F0 + (1.0 - F0) * pow(1.0 - n_dot_v, 5.0);
    reflection *= fresnel;

    frag_color = texture(u_color_map, v_uv) + vec4(reflection, 0.0);
  }
)";
