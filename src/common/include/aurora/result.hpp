
// Copyright (C) 2022 fleroviux. All rights reserved.

#pragma once

#include <aurora/log.hpp>
#include <utility>

#define AURA_RESULT_OK 0

namespace Aura {

template <typename TCode, typename TValue>
struct Result {
  Result(TCode code) : code(code) {}

  Result(TValue&& value) : code((TCode)AURA_RESULT_OK), value(std::move(value)) {}

  bool Ok() const { return (int)Code() == AURA_RESULT_OK; }

  auto Code() const -> TCode { return code; }

  auto Unwrap() -> TValue&& {
    if (!Ok()) {
      Panic("called Unwrap() on a non-ok Result");
    }

    return std::move(value);
  }

private:
  TCode code;
  TValue value;
};

} // namespace Aura
