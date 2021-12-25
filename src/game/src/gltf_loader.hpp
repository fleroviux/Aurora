/*
 * Copyright (C) 2021 fleroviux
 */

#pragma once

#include <aurora/scene/game_object.hpp>
#include <aurora/integer.hpp>
#include <filesystem>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "texture.hpp"
#include "material.hpp"

// TODO: move this somewhere more appropriate.
#include <aurora/scene/geometry/geometry.hpp>

namespace Aura {

// TODO: move this somewhere more appropriate.
struct MeshComponent final : Component {
  MeshComponent(
    GameObject* owner,
    std::shared_ptr<Geometry> geometry,
    std::shared_ptr<Material> material
  )   : Component(owner)
      , geometry(geometry)
      , material(material) {
  }

  std::shared_ptr<Geometry> geometry;
  std::shared_ptr<Material> material;
};

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
    int buffer_view;
    int count;
    VertexBufferLayout::Attribute attribute;
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
  auto load_primitive_idx(nlohmann::json const& primitive) -> IndexBuffer;
  auto load_primitive_vtx(nlohmann::json const& primitive) -> std::vector<VertexBuffer>;
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
  std::vector<std::shared_ptr<Texture>> images_;
  std::vector<std::shared_ptr<Material>> materials_;
};

} // namespace Aura
