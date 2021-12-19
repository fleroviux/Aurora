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
  auto scene = new GameObject{"Scene"};

  base_path_ = std::filesystem::path{path}.remove_filename();

  load_buffers(gltf);
  load_buffer_views(gltf);
  load_accessors(gltf);
  load_meshes(gltf);

  // quick test:
  scene->add_component<MeshComponent>(meshes_[0].primitives[0].geometry);

  return scene;
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
        auto primitive_out = Mesh::Primitive{};

        // The renderer does not support non-indexed geometry yet.
        Assert(primitive.contains("indices"), "GLTFLoader: non-indexed geometry is unsupported");

        // TODO: sanity/bounds checks and all.
        auto const& accessor = accessors_[primitive["indices"].get<int>()];
        auto const& buffer_view = buffer_views_[accessor.buffer_view];
        auto const& attribute = accessor.attribute;
        auto data_type = IndexDataType{};
        size_t bytes_per_index = 2;

        switch (attribute.data_type) {
          case VertexDataType::UInt16: data_type = IndexDataType::UInt16; break;
          case VertexDataType::UInt32: data_type = IndexDataType::UInt32; bytes_per_index = 4; break;
          default: 
            Assert(false, "GLTFLoader: bad index accessor data type, must be uint16 or uint32");
        }

        auto index_buffer = IndexBuffer{data_type, accessor.count};
        std::memcpy(index_buffer.data(), buffer_view.data, accessor.count * bytes_per_index);

        std::vector<VertexBuffer> buffers;

        {
          auto const& accessor = accessors_[primitive["attributes"]["POSITION"].get<int>()];
          auto const& buffer_view = buffer_views_[accessor.buffer_view];
          auto const& attribute = accessor.attribute;

          auto byte_stride = buffer_view.byte_stride;

          // Handle non-interleaved buffers (stride = 0)
          // TODO: check if this is robust enough
          if (byte_stride == 0) {
            byte_stride = buffer_view.byte_length / accessor.count;
          }

          auto layout = VertexBufferLayout{
            .stride = byte_stride,
            .attributes = {{
              .index = 0,
              .data_type = attribute.data_type,
              .components = attribute.components,
              .normalized = attribute.normalized,
              .offset = attribute.offset
            }}
          };

          auto buffer = VertexBuffer{layout, accessor.count};
          std::memcpy(buffer.data(), buffer_view.data, buffer_view.byte_length);

          buffers.push_back(buffer);
        }

        primitive_out.geometry = new Geometry{index_buffer, buffers};

        mesh_out.primitives.push_back(primitive_out);
      }

      meshes_.push_back(mesh_out);
    }
  }
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
