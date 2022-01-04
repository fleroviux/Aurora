/*
 * Copyright (C) 2021 fleroviux
 */

#pragma once

#include <aurora/integer.hpp>
#include <aurora/updatable.hpp>
#include <vector>

namespace Aura {

enum class IndexDataType {
  UInt16,
  UInt32
};

struct IndexBuffer : Updatable {
  IndexBuffer(
    IndexDataType data_type,
    std::vector<u8>&& buffer
  )   : data_type_(data_type)
      , buffer_(std::move(buffer)) {
  }

  IndexBuffer(
    IndexDataType data_type,
    size_t index_count
  )   : data_type_(data_type) {
    size_t buffer_size = index_count;

    switch (data_type) {
      case IndexDataType::UInt16: {
        buffer_size *= sizeof(u16);
        break;
      }
      case IndexDataType::UInt32: {
        buffer_size *= sizeof(u32);
        break;
      }
    }

    buffer_.resize(buffer_size);
  }

  auto data_type() const -> IndexDataType {
    return data_type_;
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
  IndexDataType data_type_;
  std::vector<u8> buffer_;
};

} // namespace Aura
