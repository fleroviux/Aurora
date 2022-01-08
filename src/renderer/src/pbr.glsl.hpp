/*
 * Copyright (C) 2021 fleroviux
 */

#pragma once

constexpr auto pbr_vert = R"(\
  #version 420 core

  layout (location = 0) in vec3 a_position;
  layout (location = 1) in vec3 a_normal;
  layout (location = 2) in vec2 a_uv;
  layout (location = 3) in vec3 a_color;

  layout (binding = 0, std140) uniform Camera {
    mat4 u_view;
    mat4 u_projection;
  };

  layout (binding = 1, std140) uniform Material {
    mat4 u_model;
    float u_metalness;
    float u_roughness;
  };

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

constexpr auto pbr_frag = R"(\
  #version 420 core

  #define PI  3.14159265358
  #define TAU 6.28318530717

  // PBR library begin

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

  // SH library begin

  const vec3 SH[9] = vec3[9](
    vec3(1.2526156902313232, 1.2436836957931519, 1.2180569171905518),
    vec3(0.12890386581420898, 0.3892329931259155, 0.727676510810852),
    vec3(0.09071195870637894, 0.0888092964887619, 0.08513105660676956),
    vec3(-0.09755872189998627, -0.08879192918539047, -0.07212600857019424),
    vec3(-0.1784418523311615, -0.1668754369020462, -0.13851317763328552),
    vec3(0.22431527078151703, 0.19814930856227875, 0.15394988656044006),
    vec3(-0.23336298763751984, -0.22071699798107147, -0.2437698394060135),
    vec3(-0.1290844827890396, -0.11162897199392319, -0.09100642800331116),
    vec3(-0.44091033935546875, -0.40154850482940674, -0.4266403019428253)
  );

  vec3 CalculateSH(in vec3 sh[9], vec3 normal) {
    // l = 0
    const float c0 = 0.28209479177 * 0.00600642757;

    // l = 1
    const float c1 = 0.48860251190 * 1.53498363494;

    // l = 2
    const float c2 = 1.09254843059 * 0.00387886399;
    const float c3 = 0.31539156525 * 0.00387886399;
    const float c4 = 0.54627421529 * 0.00387886399;

    vec3 result = vec3(0.0);

    result += c0 * sh[0];

    result += c1 * normal.y * sh[1];
    result += c1 * normal.z * sh[2];
    result += c1 * normal.x * sh[3];

    result += c2 * normal.x * normal.y * sh[4];
    result += c2 * normal.y * normal.z * sh[5];
    result += c3 * (-normal.x * normal.x - normal.y * normal.y + 2.0 * normal.z * normal.z) * sh[6];
    result += c2 * normal.z * normal.x * sh[7];
    result += c4 * (normal.x * normal.x - normal.y * normal.y) * sh[8];

    return result;
  }

  // SH library end

  layout (location = 0) out vec4 frag_color;

  in vec3 v_world_position;
  in vec3 v_world_normal;
  in vec3 v_view_position;
  in vec3 v_color;
  in vec2 v_uv;
  in vec3 v_normal;

  layout (binding = 0, std140) uniform Camera {
    mat4 u_view;
    mat4 u_projection;
  };

  layout (binding = 1, std140) uniform Material {
    mat4 u_model;
    float u_metalness;
    float u_roughness;
  };

  layout (binding = 0) uniform sampler2D u_diffuse_map;
  layout (binding = 1) uniform sampler2D u_metalness_map;
  layout (binding = 2) uniform sampler2D u_roughness_map;
  layout (binding = 3) uniform sampler2D u_normal_map;

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

    result += CalculateSH(SH, geometry.normal) * (1.0 - geometry.metalness) * geometry.albedo;

    result = ACESFilm(result);
    result = LinearTosRGB(result);

    frag_color = vec4(result, 1.0);
  }
)";
