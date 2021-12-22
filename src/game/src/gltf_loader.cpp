/*
 * Copyright (C) 2021 fleroviux
 */

#include <aurora/log.hpp>
#include <fstream>

#include "gltf_loader.hpp"

namespace Aura {

auto GLTFLoader::parse(std::string const& path) -> GameObject* {
  auto file = std::fstream{path};

  if (!file.good()) {
    // ...
  }

  std::string json;
  json.reserve(std::filesystem::file_size(path));
  json.assign(std::istream_iterator<char>(file), std::istream_iterator<char>());

  auto gltf = nlohmann::json::parse(json);

  base_path_ = std::filesystem::path{path}.remove_filename();

  load_buffers(gltf);
  load_buffer_views(gltf);
  load_images(gltf);
  load_materials(gltf);
  load_accessors(gltf);
  load_meshes(gltf);

  // TODO: respect default scene.
  if (gltf.contains("scenes")) {
    return load_scene(gltf, 0);
  }

  // TODO: throw an exception or something?
  return nullptr;
}

void GLTFLoader::load_buffers(nlohmann::json const& gltf) {
  /**
   * TODO:
   *   - URI is not a required field (probably only matters for GLBs?)
   *   - URI can be a data-URI but we assume a file path right now
   */

  if (gltf.contains("buffers")) {
    for (auto const& buffer : gltf["buffers"]) {
      auto uri = buffer["uri"].get<std::string>();
      auto byte_length = buffer["byteLength"].get<int>();

      uri = base_path_ / uri;

      std::ifstream file{uri, std::ios::binary};

      Assert(file.good(), "GLTFLoader: failed to load buffer: {}", uri);

      size_t real_length = std::filesystem::file_size(uri);

      Assert(real_length >= byte_length, "GLTFLoader: buffer was smaller than specified");

      auto buffer_out = Buffer{};
      buffer_out.resize(byte_length);
      file.read((char*)buffer_out.data(), byte_length);
      file.close();
      buffers_.push_back(buffer_out);

      Log<Info>("GLTFLoader: successfully read buffer {} ({} bytes)", uri, byte_length);
    }
  }
}

void GLTFLoader::load_buffer_views(nlohmann::json const& gltf) {
  if (gltf.contains("bufferViews")) {
    for (auto const& buffer_view : gltf["bufferViews"]) {
      auto buffer_id = buffer_view["buffer"].get<int>();
      auto const& buffer = buffers_[buffer_id];

      Assert(buffer_id < buffers_.size(), "GLTFLoader: buffer ID was out-of-range");

      auto byte_offset = size_t{};
      auto byte_stride = size_t{};
      auto byte_length = buffer_view["byteLength"].get<int>();

      if (buffer_view.contains("byteOffset")) {
        byte_offset = buffer_view["byteOffset"].get<int>();
      }

      if (buffer_view.contains("byteStride")) {
        byte_stride = buffer_view["byteStride"].get<int>();
      }

      Assert((byte_length + byte_offset) <= buffer.size(),
        "GLTFLoader: buffer views was out-of-bounds of the underlying buffer");

      auto buffer_view_out = BufferView{
        .data = (u8 const*)&buffer[byte_offset],
        .byte_length = byte_length,
        .byte_stride = byte_stride
      };

      Log<Info>("GLTFLoader: parsed bufferView[{}] buffer={} byte_offset={} byte_length={} byte_stride={}",
        buffer_views_.size(), buffer_id, byte_offset, byte_length, byte_stride);

      buffer_views_.push_back(buffer_view_out);
    }
  }
}

void GLTFLoader::load_accessors(nlohmann::json const& gltf) {
  if (gltf.contains("accessors")) {
    for (auto const& accessor : gltf["accessors"]) {
      auto accessor_out = Accessor{};

      // TODO: bufferView is technically not required (only GLBs?)
      accessor_out.buffer_view = accessor["bufferView"].get<int>();

      if (accessor.contains("byteOffset")) {
        accessor_out.attribute.offset = accessor["byteOffset"].get<int>();
      }

      accessor_out.attribute.data_type = to_vertex_data_type(accessor["componentType"].get<int>());
      accessor_out.attribute.components = to_component_count(accessor["type"].get<std::string>());
      accessor_out.count = accessor["count"].get<int>();

      if (accessor.contains("normalized")) {
        accessor_out.attribute.normalized = accessor["normalized"].get<bool>();
      } else {
        accessor_out.attribute.normalized = false;
      }

      Assert(!accessor.contains("sparse"), "GLTFLoader: unsupported sparse accessor");

      accessors_.push_back(accessor_out);
    }
  }
}

void GLTFLoader::load_meshes(nlohmann::json const& gltf) {
  if (gltf.contains("meshes")) {
    for (auto const& mesh : gltf["meshes"]) {
      auto mesh_out = Mesh{};

      for (auto const& primitive : mesh["primitives"]) {
        Assert(primitive.contains("indices"),
          "GLTFLoader: non-indexed geometry is unsupported");

        // TODO: material is not a required field - fallback to a standard material.
        mesh_out.primitives.push_back(Mesh::Primitive{
          .geometry = new Geometry{
            load_primitive_idx(primitive),
            load_primitive_vtx(primitive)
          },
          .material = materials_[primitive["material"].get<int>()]
        });
      }

      meshes_.push_back(mesh_out);
    }
  }
}

auto GLTFLoader::load_primitive_idx(
  nlohmann::json const& primitive
) -> IndexBuffer {
  auto const& accessor = accessors_[primitive["indices"].get<int>()];
  auto const& buffer_view = buffer_views_[accessor.buffer_view];
  auto const& attribute = accessor.attribute;

  auto data_type = IndexDataType{};
  auto data_width = size_t{};

  switch (attribute.data_type) {
    case VertexDataType::UInt16:
      data_type = IndexDataType::UInt16;
      data_width = sizeof(u16);
      break;
    case VertexDataType::UInt32:
      data_type = IndexDataType::UInt32;
      data_width = sizeof(u32);
      break;
    default: 
      Assert(false, "GLTFLoader: bad index accessor data type, must be uint16 or uint32");
  }

  auto buffer = IndexBuffer{data_type, accessor.count};

  std::memcpy(buffer.data(), buffer_view.data, accessor.count * data_width);

  return buffer;
}

auto GLTFLoader::load_primitive_vtx(
  nlohmann::json const& primitive
) -> std::vector<VertexBuffer> {
  auto const& attributes = primitive["attributes"];
  
  auto buffers = std::vector<VertexBuffer>{};
  VertexBufferLayout* layout_table[buffer_views_.size()];

  for (auto& v : layout_table) v = nullptr;

  const auto add_attribute = [&](char const* name, int index) {
    if (!attributes.contains(name)) {
      return;
    }

    auto const& accessor = accessors_[attributes[name].get<int>()];
    auto const& attribute = accessor.attribute;

    auto layout = layout_table[accessor.buffer_view];

    if (layout == nullptr) {
      auto const& buffer_view = buffer_views_[accessor.buffer_view];

      auto byte_stride = buffer_view.byte_stride;

      // Handle non-interleaved buffers (stride = 0)
      // TODO: check if this is robust enough
      if (byte_stride == 0) {
        byte_stride = buffer_view.byte_length / accessor.count;
      }

      auto buffer = VertexBuffer{VertexBufferLayout{
        .stride = byte_stride,
        .attributes = {}
      }, accessor.count};

      std::memcpy(buffer.data(), buffer_view.data, buffer_view.byte_length);

      buffers.push_back(std::move(buffer));
      layout = &buffers.back().layout();
      layout_table[accessor.buffer_view] = layout; 
    }

    layout->attributes.push_back(VertexBufferLayout::Attribute{
      .index = index,
      .data_type = attribute.data_type,
      .components = attribute.components,
      .normalized = attribute.normalized,
      .offset = attribute.offset
    });
  };

  add_attribute("POSITION", 0);
  add_attribute("NORMAL", 1);
  add_attribute("TEXCOORD_0", 2);
  add_attribute("COLOR_0", 3);

  return buffers;
}

void GLTFLoader::load_images(nlohmann::json const& gltf) {
  if (gltf.contains("images")) {
    for (auto const& image : gltf["images"]) {
      Assert(image.contains("uri"),
        "GLTFLoader: loading textures from buffers is not supported");

      auto uri = image["uri"].get<std::string>();

      // TODO: URI can be a data-URI but we assume a file path right now
      images_.push_back(Texture::load(base_path_ / uri));

      Log<Info>("GLTFLoader: loaded image: {}", uri);
    }
  }
}

void GLTFLoader::load_materials(nlohmann::json const& gltf) {
  if (gltf.contains("materials")) {
    for (auto const& material : gltf["materials"]) {
      auto material_out = new Material{};

      if (material.contains("pbrMetallicRoughness")) {
        auto const& pbr = material["pbrMetallicRoughness"];

        if (pbr.contains("baseColorTexture")) {
          auto tid = pbr["baseColorTexture"]["index"].get<int>();
          material_out->albedo = images_[gltf["textures"][tid]["source"].get<int>()];
        }
      }

      materials_.push_back(material_out);
    }
  }
}

auto GLTFLoader::load_node(nlohmann::json const& nodes, size_t id) -> GameObject* {
  auto object = new GameObject{};

  auto const& node = nodes[id];

  // TODO: use transform and name information.

  if (node.contains("mesh")) {
    auto const& mesh = meshes_[node["mesh"].get<int>()];

    if (mesh.primitives.size() == 1) {
      object->add_component<MeshComponent>(mesh.primitives[0].geometry, mesh.primitives[0].material);
    } else {
      for (auto const& primitive : mesh.primitives) {
        auto child = new GameObject{};
        child->add_component<MeshComponent>(primitive.geometry, primitive.material);
        object->add_child(child);
      }
    }
  }

  if (node.contains("children")) {
    for (auto const& child : node["children"]) {
      object->add_child(load_node(nodes, child.get<int>()));
    }
  }

  return object;
}

auto GLTFLoader::load_scene(nlohmann::json const& gltf, size_t id) -> GameObject* {
  auto object = new GameObject{};
  auto const& scene = gltf["scenes"][id];

  // TODO: do something with the scene name

  if (scene.contains("nodes")) {
    for (auto const& node : scene["nodes"]) {
      object->add_child(load_node(gltf["nodes"], node.get<int>()));
    }
  }

  return object;
}

auto GLTFLoader::to_vertex_data_type(int component_type) -> VertexDataType {
  switch (component_type) {
    case 5120: return VertexDataType::SInt8;
    case 5121: return VertexDataType::UInt8;
    case 5122: return VertexDataType::SInt16;
    case 5123: return VertexDataType::UInt16;
    case 5125: return VertexDataType::UInt32;
    case 5126: return VertexDataType::Float32;
  }

  Assert(false, "GLTFLoader: unsupported accessor component type: {}", component_type);
}

auto GLTFLoader::to_component_count(std::string type) -> int {
  if (type == "SCALAR") return 1;
  if (type == "VEC2") return 2;
  if (type == "VEC3") return 3;
  if (type == "VEC4") return 4;
  if (type == "MAT2") return 4;
  if (type == "MAT3") return 9;
  if (type == "MAT4") return 16;

  Assert(false, "GLTFLoader: unsupported accessor type: {}", type);
}

static auto to_topology(int mode) -> Geometry::Topology {
  if (mode == 4) return Geometry::Topology::Triangles;

  Assert(false, "GLTFLoader: unsupported primitive mode: {}", mode);
}

} // namespace Aura
