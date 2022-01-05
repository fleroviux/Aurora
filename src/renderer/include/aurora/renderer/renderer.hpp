/*
 * Copyright (C) 2021 fleroviux
 */

#include <aurora/renderer/component/camera.hpp>
#include <aurora/renderer/component/mesh.hpp>
#include <aurora/renderer/component/scene.hpp>
#include <aurora/renderer/uniform_block.hpp>
#include <aurora/scene/game_object.hpp>
#include <GL/glew.h>
#include <optional>

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

  void update_camera_transform(GameObject* camera);

  void upload_geometry(Geometry* geometry, GeometryCacheEntry& data);

  void bind_uniform_block(
    UniformBlock& uniform_block,
    GLuint program,
    size_t binding
  );
  void bind_texture(Texture* texture, GLenum slot);

  void bind_material(Material* material, GameObject* object);
  void draw_geometry(Geometry* geometry);

  void create_geometry(Geometry* geometry);

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

  void create_default_program();

  static auto compile_shader(
    GLenum type,
    char const* source
  ) -> std::optional<GLuint>;
  
  static auto compile_program(
    char const* vert_src,
    char const* frag_src
  ) -> std::optional<GLuint>;

  static auto get_gl_attribute_type(VertexDataType data_type) -> GLenum;

  GLuint program;

  UniformBlock uniform_camera;

  std::unordered_map<Geometry const*, GeometryCacheEntry> geometry_cache_;
  std::unordered_map<UniformBlock const*, GLuint> uniform_block_cache_;
  std::unordered_map<Texture const*, GLuint> texture_cache_;

  float gl_max_anisotropy;
};

} // namespace Aura

