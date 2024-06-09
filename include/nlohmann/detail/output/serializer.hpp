//     __ _____ _____ _____
//  __|  |   __|     |   | |  JSON for Modern C++
// |  |  |__   |  |  | | | |  version 3.11.3
// |_____|_____|_____|_|___|  https://github.com/nlohmann/json
//
// SPDX-FileCopyrightText: 2008-2009 Björn Hoehrmann <bjoern@hoehrmann.de>
// SPDX-FileCopyrightText: 2013-2023 Niels Lohmann <https://nlohmann.me>
// SPDX-License-Identifier: MIT

#pragma once

#include <array>
#include <clocale>
#include <cstdint>
#include <memory>
#include <string>
#include <type_traits>
#include "error_handler_t.h"

namespace nlohmann {
class ordered_json;

namespace detail {
class output_adapter_protocol;

class serializer
{
    using binary_char_t = uint8_t;
    static constexpr uint8_t UTF8_ACCEPT = 0;
    static constexpr uint8_t UTF8_REJECT = 1;

public:
    serializer(std::shared_ptr<output_adapter_protocol> s, const char ichar,
               error_handler_t error_handler_ = error_handler_t::strict);

    serializer(const serializer &) = delete;
    serializer &operator=(const serializer &) = delete;
    serializer(serializer &&) = delete;
    serializer &operator=(serializer &&) = delete;
    ~serializer() = default;

    void dump(const ordered_json &val, const bool pretty_print, const bool ensure_ascii,
              const unsigned int indent_step, const unsigned int current_indent = 0);

#ifndef JSON_TESTS_PRIVATE
private:
#endif
    void dump_escaped(const std::string &s, const bool ensure_ascii);

private:
    unsigned int count_digits(uint64_t x) noexcept;
    static std::string hex_bytes(uint8_t byte);
    void dump_integer(int64_t x);

    void dump_float(double x);
    void dump_float(double x, std::true_type /*is_ieee_single_or_double*/);
    void dump_float(double x, std::false_type /*is_ieee_single_or_double*/);

    static uint8_t decode(uint8_t &state, std::uint32_t &codep, const uint8_t byte) noexcept;

private:
    std::shared_ptr<output_adapter_protocol> o = nullptr;
    std::array<char, 64> number_buffer{{}};
    const std::lconv *loc = nullptr;
    const char thousands_sep = '\0';
    const char decimal_point = '\0';
    std::array<char, 512> string_buffer{{}};
    const char indent_char;
    std::string indent_string;
    const error_handler_t error_handler;
};

} // namespace detail
} // namespace nlohmann
