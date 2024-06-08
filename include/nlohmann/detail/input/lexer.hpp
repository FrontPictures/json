//     __ _____ _____ _____
//  __|  |   __|     |   | |  JSON for Modern C++
// |  |  |__   |  |  | | | |  version 3.11.3
// |_____|_____|_____|_|___|  https://github.com/nlohmann/json
//
// SPDX-FileCopyrightText: 2013-2023 Niels Lohmann <https://nlohmann.me>
// SPDX-License-Identifier: MIT

#pragma once

#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <initializer_list>
#include <memory>
#include <string>
#include <vector>

#include "input_format_t.h"
#include "position_t.hpp"

namespace nlohmann {
namespace detail {
class iterator_input_adapter;

class lexer
{
public:
    explicit lexer(const iterator_input_adapter &adapter, bool ignore_comments_ = false) noexcept;
    ~lexer();

private:
    int get_codepoint();
    bool next_byte_in_range(std::initializer_list<uint64_t> ranges);
    lexer_token_type scan_string();
    bool scan_comment();
    static void strtof(float &f, const char *str, char **endptr) noexcept;
    static void strtof(double &f, const char *str, char **endptr) noexcept;
    static void strtof(long double &f, const char *str, char **endptr) noexcept;

    lexer_token_type scan_number();
    lexer_token_type scan_literal(const char *literal_text, const size_t length, lexer_token_type return_type);
    void reset() noexcept;
    uint64_t get();
    void unget();
    void add(uint64_t c);

public:
    constexpr int64_t get_number_integer() const noexcept { return value_integer; }
    constexpr double get_number_float() const noexcept { return value_float; }
    std::string &get_string() { return token_buffer; }
    constexpr position_t get_position() const noexcept { return position; }
    std::string get_token_string() const;
    constexpr const char *get_error_message() const noexcept { return error_message; }
    bool skip_bom();
    void skip_whitespace();
    lexer_token_type scan();

private:
    std::unique_ptr<iterator_input_adapter> ia;
    const bool ignore_comments = false;
    uint64_t current = uint64_t(EOF);
    bool next_unget = false;
    position_t position{};
    std::vector<char> token_string{};
    std::string token_buffer{};
    const char *error_message = "";
    int64_t value_integer = 0;
    double value_float = 0;
    const uint64_t decimal_point_char = '.';
};

} // namespace detail
} // namespace nlohmann
