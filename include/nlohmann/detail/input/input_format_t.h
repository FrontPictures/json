#ifndef INPUT_FORMAT_T_H
#define INPUT_FORMAT_T_H

namespace nlohmann {
namespace detail {

inline bool little_endianness(int num = 1) noexcept
{
    return *reinterpret_cast<char *>(&num) == 1;
}

enum class input_format_t { json, cbor, msgpack, ubjson, bson, bjdata };
enum class cbor_tag_handler_t { error, ignore, store };

enum class lexer_token_type {
    uninitialized,
    literal_true,
    literal_false,
    literal_null,
    value_string,
    value_integer,
    value_float,
    begin_array,
    begin_object,
    end_array,
    end_object,
    name_separator,
    value_separator,
    parse_error,
    end_of_input,
    literal_or_value
};

const char *lexer_token_type_name(const lexer_token_type t) noexcept;

} // namespace detail
} // namespace nlohmann

#endif // INPUT_FORMAT_T_H
