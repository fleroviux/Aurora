
// Copyright (C) 2022 fleroviux. All rights reserved.

#pragma once

#include <aurora/renderer/component/mesh.hpp>
#include <aurora/scene/game_object.hpp>
#include <aurora/integer.hpp>
#include <filesystem>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace Aura {

struct GLTFLoader {
  auto parse(std::string const& path) -> GameObject*;

private:
  using Buffer = std::vector<u8>;

  struct BufferView {
    u8 const* data;
    size_t byte_length;
    size_t byte_stride = 0;
  };

  struct Accessor {
    size_t buffer_view;
    size_t count;
    Geometry::Attribute attribute;
  };

  struct Mesh {
    struct Primitive {
      std::shared_ptr<Geometry> geometry;
      std::shared_ptr<Material> material;
    };
    std::vector<Primitive> primitives;
  };

  void load_buffers(nlohmann::json const& gltf);
  void load_buffer_views(nlohmann::json const& gltf);
  void load_accessors(nlohmann::json const& gltf);
  void load_meshes(nlohmann::json const& gltf);
  void load_primitive_idx(nlohmann::json const& primitive, std::shared_ptr<Geometry>& geometry);
  void load_primitive_vtx(nlohmann::json const& primitive, std::shared_ptr<Geometry>& geometry);
  void load_images(nlohmann::json const& gltf);
  void load_materials(nlohmann::json const& gltf);
  auto load_node(nlohmann::json const& nodes, size_t id) -> GameObject*;
  auto load_scene(nlohmann::json const& gltf, size_t id) -> GameObject*;

  static auto to_vertex_data_type(int component_type) -> VertexDataType;
  static auto to_component_count(std::string type) -> int;
  static auto to_topology(int mode) -> Geometry::Topology;

  std::filesystem::path base_path_;
  std::vector<Buffer> buffers_;
  std::vector<BufferView> buffer_views_;
  std::vector<Accessor> accessors_;
  std::vector<Mesh> meshes_;
  std::vector<std::shared_ptr<Texture2D>> images_;
  std::vector<std::shared_ptr<Material>> materials_;
};

} // namespace Aura
