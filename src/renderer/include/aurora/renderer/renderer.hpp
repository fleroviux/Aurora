/*
 * Copyright (C) 2021 fleroviux
 */

#include <aurora/renderer/component/camera.hpp>
#include <aurora/renderer/component/mesh.hpp>
#include <aurora/renderer/component/scene.hpp>
#include <aurora/renderer/uniform_block.hpp>
#include <aurora/scene/game_object.hpp>
#include <aurora/utility.hpp>
#include <GL/glew.h>
#include <optional>
#include <typeindex>

namespace Aura {

struct OpenGLRenderer {
  OpenGLRenderer();
 ~OpenGLRenderer();

  void render(GameObject* scene);

private:
  struct GeometryCacheEntry {
    GLuint vao;
    GLuint ibo;
    std::vector<GLuint> vbos;
  };

  using ProgramCacheKey = std::pair<std::type_index, u32>;

  void update_camera_transform(GameObject* camera);

  void bind_uniform_block(
    UniformBlock& uniform_block,
    GLuint program,
    size_t binding
  );
  void bind_texture(Texture* texture, GLenum slot);
  void bind_material(Material* material, GameObject* object);
  void draw_geometry(Geometry* geometry);

  void create_geometry(Geometry* geometry);
  auto get_program(Material* material) -> GLuint;

  void update_geometry_ibo(
    Geometry* geometry,
    GeometryCacheEntry& data,
    bool force_update
  );

  void update_geometry_vbo(
    Geometry* geometry,
    GeometryCacheEntry& data,
    bool force_update
  );

  static auto compile_shader(
    GLenum type,
    char const* source
  ) -> std::optional<GLuint>;
  
  static auto compile_program(
    char const* vert_src,
    char const* frag_src
  ) -> std::optional<GLuint>;

  static auto get_gl_attribute_type(VertexDataType data_type) -> GLenum;

  UniformBlock uniform_camera;

  std::unordered_map<Geometry const*, GeometryCacheEntry> geometry_cache_;
  std::unordered_map<UniformBlock const*, GLuint> uniform_block_cache_;
  std::unordered_map<Texture const*, GLuint> texture_cache_;
  std::unordered_map<ProgramCacheKey, GLuint, pair_hash> program_cache_;

  float gl_max_anisotropy;
};

} // namespace Aura

