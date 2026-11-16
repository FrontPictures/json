//     __ _____ _____ _____
//  __|  |   __|     |   | |  JSON for Modern C++
// |  |  |__   |  |  | | | |  version 3.11.3
// |_____|_____|_____|_|___|  https://github.com/nlohmann/json
//
// SPDX-FileCopyrightText: 2013-2023 Niels Lohmann <https://nlohmann.me>
// SPDX-License-Identifier: MIT

#pragma once

#include <cstddef>

namespace nlohmann {
class ordered_json;
namespace detail {

size_t combine(size_t seed, size_t h) noexcept;
size_t hash(const ordered_json &j);

} // namespace detail
} // namespace nlohmann
