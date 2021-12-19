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

      Assert(file.good(), "rip");

      Log<Info>("GLTFLoader: read buffer {}", uri);

      id++;
    }
  }
}

} // namespace Aura
