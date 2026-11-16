//     __ _____ _____ _____
//  __|  |   __|     |   | |  JSON for Modern C++
// |  |  |__   |  |  | | | |  version 3.11.3
// |_____|_____|_____|_|___|  https://github.com/nlohmann/json
//
// SPDX-FileCopyrightText: 2013-2023 Niels Lohmann <https://nlohmann.me>
// SPDX-License-Identifier: MIT

#pragma once

#include <memory>
#include <string>
#include "input_adapters.hpp"
#include "parser_types.h"
#include "input_format_t.h"

namespace nlohmann {
class ordered_json;
class json_sax;
namespace detail {
class lexer;

class parser
{
public:
    explicit parser(const iterator_input_adapter &adapter, const parser_callback_t cb = nullptr,
                    const bool allow_exceptions_ = true, const bool skip_comments = false);
    ~parser();

    void parse(const bool strict, ordered_json &result);
    bool accept(const bool strict = true);
    bool sax_parse(json_sax *sax, const bool strict = true);

private:
    bool sax_parse_internal(json_sax *sax);
    lexer_token_type get_token();
    std::string exception_message(const lexer_token_type expected, const std::string &context);

private:
    const parser_callback_t callback = nullptr;
    lexer_token_type last_token = lexer_token_type::uninitialized;
    std::unique_ptr<lexer> m_lexer;
    const bool allow_exceptions = true;
};

} // namespace detail
} // namespace nlohmann
