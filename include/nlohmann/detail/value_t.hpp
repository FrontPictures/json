//     __ _____ _____ _____
//  __|  |   __|     |   | |  JSON for Modern C++
// |  |  |__   |  |  | | | |  version 3.11.3
// |_____|_____|_____|_|___|  https://github.com/nlohmann/json
//
// SPDX-FileCopyrightText: 2013-2023 Niels Lohmann <https://nlohmann.me>
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>
#include <compare>

namespace nlohmann {
namespace detail {

enum class value_t : uint8_t {
    null,
    object,
    array,
    string,
    boolean,
    number_integer,
    number_float,
    binary,
    discarded
};

std::partial_ordering operator<=>(const value_t lhs, const value_t rhs) noexcept;

#if defined(__GNUC__)
inline bool operator<(const value_t lhs, const value_t rhs) noexcept
{
    return std::is_lt(lhs <=> rhs);
}
#endif

} // namespace detail
} // namespace nlohmann
