/*
 * Copyright (C) 2021 fleroviux
 */

#pragma once

#include <aurora/math/matrix4.hpp>
#include <aurora/math/vector.hpp>
#include <aurora/integer.hpp>
#include <aurora/log.hpp>
#include <fmt/format.h>
#include <string>
#include <type_traits>
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

  bool operator==(UniformTypeInfo const& rhs) {
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
  struct Member {
    std::string name;
    UniformTypeInfo type_info;
    size_t position;
    size_t count;

    // TODO: this is mostly just for debugging, remove later.
    size_t size;
    size_t alignment;
  };

  template<typename T>
  void add(std::string const& name, size_t count = 0) {
    auto type_info = UniformTypeInfo{};

    // TODO: improve on this type decoding mechanism.
    if constexpr (std::is_same_v<T, float>) {
      type_info = UniformTypeInfo{
        .type = UniformTypeInfo::Type::F32,
        .grade = UniformTypeInfo::Grade::Scalar
      };
    }
    if constexpr (std::is_same_v<T, Vector2>) {
      type_info = UniformTypeInfo{
        .type = UniformTypeInfo::Type::F32,
        .grade = UniformTypeInfo::Grade::Vec2
      };
    }
    if constexpr (std::is_same_v<T, Vector3>) {
      type_info = UniformTypeInfo{
        .type = UniformTypeInfo::Type::F32,
        .grade = UniformTypeInfo::Grade::Vec3
      };
    }
    if constexpr (std::is_same_v<T, Vector4>) {
      type_info = UniformTypeInfo{
        .type = UniformTypeInfo::Type::F32,
        .grade = UniformTypeInfo::Grade::Vec4
      };
    }
    if constexpr (std::is_same_v<T, Matrix4>) {
      type_info = UniformTypeInfo{
        .type = UniformTypeInfo::Type::F32,
        .grade = UniformTypeInfo::Grade::Mat4
      };
    }

    auto [size, alignment] = get_size_and_alignment(type_info, count);

    auto remainder = position % alignment;
    if (remainder != 0) {
      position += alignment - remainder;
    }

    members_.push_back(Member{
      .name = name,
      .type_info = type_info,
      .position = position,
      .count = count,
      .size = size,
      .alignment = alignment
    });

    position += size;
  }

  auto to_string() const -> std::string {
    std::string text;

    for (auto& member : members_) {
      text += fmt::format("{}{} {} (position: {}, size: {}, alignment: {})\n",
        member.type_info.to_string(),
        member.count > 1 ? fmt::format("[{}]", member.count) : "",
        member.name,
        member.position,
        member.size,
        member.alignment
      );
    }

    return text;
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
//   template<typename T>
//   struct TypeInfo {
//   };

// #define DECLARE_TYPE_INFO(T, type_, grade_) \
//   template<>\
//   struct TypeInfo<T> {\
//     static constexpr Type type = type_;\
//     static constexpr Grade grade = grade_;\
//   };

//   DECLARE_TYPE_INFO(bool, Type::Bool, Grade::Scalar);
//   DECLARE_TYPE_INFO(int, Type::SInt, Grade::Scalar);
//   DECLARE_TYPE_INFO(uint, Type::UInt, Grade::Scalar);
//   DECLARE_TYPE_INFO(float, Type::F32, Grade::Scalar);
//   DECLARE_TYPE_INFO(double, Type::F64, Grade::Scalar);
//   DECLARE_TYPE_INFO(Vector2, Type::F32, Grade::Vec2);
//   DECLARE_TYPE_INFO(Vector3, Type::F32, Grade::Vec3);
//   DECLARE_TYPE_INFO(Vector4, Type::F32, Grade::Vec4);
//   DECLARE_TYPE_INFO(Matrix4, Type::F32, Grade::Mat4);

// #undef DECLARE_TYPE_INFO

  size_t position;
  std::vector<Member> members_;
};

} // namespace Aura
