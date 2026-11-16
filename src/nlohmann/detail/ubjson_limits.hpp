#pragma once
#include <cstddef>

namespace nlohmann {
namespace detail {
// Null and boolean typed arrays have no per-element bytes. Bound expansion
// independently of their announced count, as upstream does for UBJSON.
constexpr size_t ubjson_valueless_limit = size_t(1) << 20;
} // namespace detail
} // namespace nlohmann
