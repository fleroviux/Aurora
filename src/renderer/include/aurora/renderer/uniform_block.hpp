
// Copyright (C) 2022 fleroviux. All rights reserved.

#pragma once

#include <algorithm>
#include <aurora/math/matrix4.hpp>
#include <aurora/math/vector.hpp>
#include <aurora/renderer/gpu_resource.hpp>
#include <aurora/integer.hpp>
#include <aurora/log.hpp>
#include <aurora/utility.hpp>
#include <fmt/format.h>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace Aura {

struct UniformTypeInfo {
  enum class Type {
    Bool,
    SInt,
    UInt,
    F32,
    F64
  } type;

  enum class Grade {
    Scalar,
    Vec2,
    Vec3,
    Vec4,
    Mat4
  } grade;

  bool operator==(UniformTypeInfo const& rhs) const {
    return type == rhs.type && grade == rhs.grade;
  }

  auto size() const -> size_t {
    return unit() * components();
  }

  auto alignment() const -> size_t {
    if (grade == Grade::Mat4) {
      return unit() * 4;
    }
    return size();
  }

  template<typename T>
  static constexpr auto get() -> UniformTypeInfo {
    if constexpr (std::is_same_v<T, float>) {
      return UniformTypeInfo{
        .type = UniformTypeInfo::Type::F32,
        .grade = UniformTypeInfo::Grade::Scalar
      };
    } else if constexpr (std::is_same_v<T, Vector2>) {
      return UniformTypeInfo{
        .type = UniformTypeInfo::Type::F32,
        .grade = UniformTypeInfo::Grade::Vec2
      };
    } else if constexpr (std::is_same_v<T, Vector3>) {
      return UniformTypeInfo{
        .type = UniformTypeInfo::Type::F32,
        .grade = UniformTypeInfo::Grade::Vec3
      };
    } else if constexpr (std::is_same_v<T, Vector4>) {
      return UniformTypeInfo{
        .type = UniformTypeInfo::Type::F32,
        .grade = UniformTypeInfo::Grade::Vec4
      };
    } else if constexpr (std::is_same_v<T, Matrix4>) {
      return UniformTypeInfo{
        .type = UniformTypeInfo::Type::F32,
        .grade = UniformTypeInfo::Grade::Mat4
      };
    } else {
      static_no_match();
    }
  }

  auto to_string() const -> std::string {
    if (grade == Grade::Scalar) {
      switch (type) {
        case Type::Bool: return "bool";
        case Type::SInt: return "int";
        case Type::UInt: return "uint";
        case Type::F32:  return "float";
        case Type::F64:  return "double";
      }
    } else {
      char const* prefix = "";

      switch (type) {
        case Type::Bool: prefix = "b";  break;
        case Type::SInt: prefix = "i";  break;
        case Type::UInt: prefix = "ui"; break;
        case Type::F64:  prefix = "d";  break;
      }

      switch (grade) {
        case Grade::Vec2: return fmt::format("{}vec2", prefix);
        case Grade::Vec3: return fmt::format("{}vec3", prefix);
        case Grade::Vec4: return fmt::format("{}vec4", prefix);
        case Grade::Mat4: return fmt::format("{}mat4", prefix);
      }
    }

    Assert(false, "UniformTypeInfo: unsupported combination of grade ({}) and type ({}).", (int)grade, (int)type);
  }

private:
  auto unit() const -> size_t {
    return type == Type::F64 ? 8 : 4;
  }

  auto components() const -> size_t {
    switch (grade) {
      case Grade::Scalar: return 1;
      case Grade::Vec2: return 2;
      case Grade::Vec3: return 4;
      case Grade::Vec4: return 4;
      case Grade::Mat4: return 16;
    }
  }
};

struct UniformBlockLayout {
  template<typename T>
  void add(std::string const& name, size_t count = 0) {
    auto type_info = UniformTypeInfo::get<T>();

    auto [size, alignment] = get_size_and_alignment(type_info, count);

    auto remainder = position_ % alignment;
    if (remainder != 0) {
      position_ += alignment - remainder;
    }

    members_.push_back(Member{
      .name = name,
      .type_info = type_info,
      .position = position_,
      .count = count
    });

    position_ += size;
  }

  static auto get_size_and_alignment(
    UniformTypeInfo const& type_info,
    size_t count
  ) -> std::pair<size_t, size_t> {
    constexpr size_t vec4_size = 16;

    auto size = type_info.size();
    auto alignment = type_info.alignment();

    if (count > 1) {
      auto remainder = size % vec4_size;
      if (remainder != 0) {
        size += vec4_size - remainder;
      }
      alignment = size;

      // Mat4 is treated as an array of Vec4.
      if (type_info.grade == UniformTypeInfo::Grade::Mat4) {
        alignment /= 4;
      }

      size *= count;
    }

    return std::make_pair(size, alignment);
  }

private:
  friend struct UniformBlock;

  struct Member {
    std::string name;
    UniformTypeInfo type_info;
    size_t position;
    size_t count;
  };

  size_t position_;
  std::vector<Member> members_;
};

struct UniformBlock final : GPUResource {
  using Member = UniformBlockLayout::Member;

  UniformBlock() {}

  UniformBlock(UniformBlockLayout const& layout) {
    size_ = layout.position_;
    data_ = new u8[size_];

    for (auto& member : layout.members_) {
      members_[member.name] = member;
    }
  }

 ~UniformBlock() override {
    if (data_ != nullptr) {
      delete data_;
    }
  }

  UniformBlock(UniformBlock const& other) = delete;

  UniformBlock(UniformBlock&& other) {
    operator=(std::move(other));
  }

  void operator=(UniformBlock const& other) = delete;

  void operator=(UniformBlock&& other) {
    std::swap(data_, other.data_);
    std::swap(size_, other.size_);
    std::swap(members_, other.members_);
  }

  auto data() const -> u8 const* {
    return data_;
  }

  auto size() const -> size_t {
    return size_;
  }

  template<typename T>
  auto get(std::string const& name) -> T& {
    auto match = members_.find(name);

    Assert(match != members_.end(),
      "UniformBlock: uniform '{}' does not exist", name);

    auto const& member = match->second;

    auto type_info = UniformTypeInfo::get<T>();
    Assert(member.type_info == type_info,
      "UniformBlock: uniform '{}' has type {} but was accessed as {}",
      name, member.type_info.to_string(), type_info.to_string());

    return *reinterpret_cast<T*>(data_ + member.position);
  } 

private:
  u8* data_ = nullptr;
  size_t size_;
  std::unordered_map<std::string, Member> members_;
};

} // namespace Aura
