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
namespace detail {

struct position_t
{
    size_t chars_read_total = 0;
    size_t chars_read_current_line = 0;
    size_t lines_read = 0;
    constexpr operator size_t() const { return chars_read_total; }
};

} // namespace detail
} // namespace nlohmann
