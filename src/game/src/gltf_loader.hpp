/*
 * Copyright (C) 2021 fleroviux
 */

#pragma once

#include <aurora/scene/game_object.hpp>
#include <aurora/integer.hpp>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

// TODO: move this somewhere more appropriate.
#include <aurora/scene/geometry/geometry.hpp>

namespace Aura {

// TODO: move this somewhere more appropriate.
struct MeshComponent final : Component {
  MeshComponent(GameObject* owner, Geometry* geometry)
      : Component(owner)
      , geometry(geometry) {
  }

  Geometry* geometry;
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
      std::string name;
      Geometry* geometry;
    };
    std::vector<Primitive> primitives;
  };

  void load_buffers(nlohmann::json const& gltf);
  void load_buffer_views(nlohmann::json const& gltf);
  void load_accessors(nlohmann::json const& gltf);
  void load_meshes(nlohmann::json const& gltf);

  static auto to_vertex_data_type(int component_type) -> VertexDataType;
  static auto to_component_count(std::string type) -> int;
  static auto to_topology(int mode) -> Geometry::Topology;

  std::vector<Buffer> buffers_;
  std::vector<BufferView> buffer_views_;
  std::vector<Accessor> accessors_;
  std::vector<Mesh> meshes_;
};

} // namespace Aura
