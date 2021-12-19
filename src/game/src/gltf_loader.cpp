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
  file.seekg(0, std::ios::end);
  json.reserve(file.tellg());
  file.seekg(0);
  json.assign(std::istream_iterator<char>(file), std::istream_iterator<char>());

  auto gltf = nlohmann::json::parse(json);
  auto scene = new GameObject{"Scene"};

  load_buffers(gltf);
  load_buffer_views(gltf);
  load_accessors(gltf);

  return scene;
}

void GLTFLoader::load_buffers(nlohmann::json const& gltf) {
  /**
   * TODO:
   *   - URI is not a required field (probably only matters for GLBs?)
   *   - URI can be a data-URI but we assume a file path right now
   */

  if (gltf.contains("buffers")) {
    size_t id = 0;

    for (auto const& buffer : gltf["buffers"]) {
      auto uri = buffer["uri"].get<std::string>();
      auto byte_length = buffer["byteLength"].get<int>();

      std::fstream file{uri, std::ios::binary};

      Assert(file.good(), "GLTFLoader: failed to load buffer: {}", uri);

      size_t real_length;
      file.seekg(0, std::ios::end);
      real_length = file.tellg();
      file.seekg(0);

      Assert(real_length >= byte_length, "GLTFLoader: buffer was smaller than specified");

      buffers_.push_back(Buffer{});
      buffers_[id].resize(byte_length);
      file.read((char*)buffers_[id].data(), byte_length);

      Log<Info>("GLTFLoader: successfully read buffer {}", uri);

      id++;
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

auto GLTFLoader::to_vertex_data_type(int component_type) -> VertexDataType {
  switch (component_type) {
    case 5120: return VertexDataType::SInt8;
    case 5121: return VertexDataType::UInt8;
    case 5122: return VertexDataType::SInt16;
    case 5123: return VertexDataType::UInt16;
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

} // namespace Aura
