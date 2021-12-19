/*
 * Copyright (C) 2021 fleroviux
 */

#pragma once

#include <aurora/integer.hpp>
#include <aurora/updatable.hpp>
#include <vector>

namespace Aura {

enum class VertexDataType {
  SInt8,
  UInt8,
  SInt16,
  UInt16,
  Float16,
  Float32
};

struct VertexBufferLayout {
  struct Attribute {
    VertexDataType data_type;
    size_t components;
    bool normalized;
    size_t offset;
  };

  size_t stride;
  std::vector<Attribute> attributes;
};

struct VertexBuffer : Updatable {
  VertexBuffer(
    VertexBufferLayout layout,
    std::vector<u8>&& buffer
  )   : layout_(layout)
      , buffer_(std::move(buffer)) {
  }

  VertexBuffer(
    VertexBufferLayout layout,
    size_t vertex_count
  )   : layout_(layout) {
    buffer_.resize(layout.stride * vertex_count);
  }

  auto layout() const -> VertexBufferLayout const& {
    return layout_;
  }

  auto data() const -> u8 const* {
    return buffer_.data();
  }

  auto data() -> u8* {
    return buffer_.data();
  }

  auto size() const -> size_t {
    return buffer_.size();
  }

private:
  VertexBufferLayout layout_;
  std::vector<u8> buffer_;
};

} // namespace Aura
