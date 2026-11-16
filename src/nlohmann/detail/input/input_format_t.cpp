#include "input_format_t.h"

const char *nlohmann::detail::lexer_token_type_name(const lexer_token_type t) noexcept
{
    switch (t) {
    case lexer_token_type::uninitialized: return "<uninitialized>";
    case lexer_token_type::literal_true: return "true literal";
    case lexer_token_type::literal_false: return "false literal";
    case lexer_token_type::literal_null: return "null literal";
    case lexer_token_type::value_string: return "string literal";
    case lexer_token_type::value_integer:
    case lexer_token_type::value_float: return "number literal";
    case lexer_token_type::begin_array: return "'['";
    case lexer_token_type::begin_object: return "'{'";
    case lexer_token_type::end_array: return "']'";
    case lexer_token_type::end_object: return "'}'";
    case lexer_token_type::name_separator: return "':'";
    case lexer_token_type::value_separator: return "','";
    case lexer_token_type::parse_error: return "<parse error>";
    case lexer_token_type::end_of_input: return "end of input";
    case lexer_token_type::literal_or_value: return "'[', '{', or a literal";

    default: return "unknown token";
    }
}
