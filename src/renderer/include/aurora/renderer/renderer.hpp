/*
 * Copyright (C) 2021 fleroviux
 */

#include <aurora/renderer/component/camera.hpp>
#include <aurora/renderer/component/mesh.hpp>
#include <aurora/renderer/component/scene.hpp>
#include <aurora/renderer/uniform_block_layout.hpp>
#include <aurora/scene/game_object.hpp>
#include <GL/glew.h>
#include <optional>

namespace Aura {

struct OpenGLRenderer {
  struct GeometryData {
    GLuint vao;
    GLuint ibo;
    std::vector<GLuint> vbos;
  };

  OpenGLRenderer();
 ~OpenGLRenderer();

  void render(GameObject* scene);

private:
  auto upload_texture(Texture* texture) -> GLuint;
  void upload_geometry(Geometry* geometry, GeometryData& data);
  void bind_uniform_block(
    UniformBlock const& uniform_block,
    GLuint program,
    size_t index
  );
  void bind_texture(GLenum slot, Texture* texture);
  void upload_transform_uniforms(Transform const& transform, GameObject* camera);

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

  std::unordered_map<Geometry const*, GeometryData> geometry_data_;
  std::unordered_map<UniformBlock const*, GLuint> uniform_data_;
  std::unordered_map<Texture const*, GLuint> texture_data_;
};

} // namespace Aura

