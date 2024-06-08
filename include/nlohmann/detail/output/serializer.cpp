#include "serializer.hpp"
#include <cmath>
#include "nlohmann/json.hpp"
#include "nlohmann/detail/conversions/to_chars.hpp"
#include "nlohmann/detail/string_concat.hpp"
#include "binary_writer.hpp"
#include "nlohmann/detail/output/output_adapters.hpp"

nlohmann::detail::serializer::serializer(std::shared_ptr<output_adapter_protocol> s, const char ichar,
                                         error_handler_t error_handler_)
    : o(std::move(s)), loc(std::localeconv()),
      thousands_sep(loc->thousands_sep == nullptr
                        ? '\0'
                        : std::char_traits<char>::to_char_type(*(loc->thousands_sep))),
      decimal_point(loc->decimal_point == nullptr
                        ? '\0'
                        : std::char_traits<char>::to_char_type(*(loc->decimal_point))),
      indent_char(ichar), indent_string(512, indent_char), error_handler(error_handler_)
{}

void nlohmann::detail::serializer::dump(const ordered_json &val, const bool pretty_print,
                                        const bool ensure_ascii, const unsigned int indent_step,
                                        const unsigned int current_indent)
{
    switch (val.m_data.m_type) {
    case value_t::object: {
        if (val.m_data.m_value.object->empty()) {
            o->write_characters("{}", 2);
            return;
        }

        if (pretty_print) {
            o->write_characters("{\n", 2);

            const auto new_indent = current_indent + indent_step;
            if (indent_string.size() < new_indent) {
                indent_string.resize(indent_string.size() * 2, ' ');
            }

            auto i = val.m_data.m_value.object->cbegin();
            for (size_t cnt = 0; cnt < val.m_data.m_value.object->size() - 1; ++cnt, ++i) {
                o->write_characters(indent_string.c_str(), new_indent);
                o->write_character('\"');
                dump_escaped(i->first, ensure_ascii);
                o->write_characters("\": ", 3);
                dump(i->second, true, ensure_ascii, indent_step, new_indent);
                o->write_characters(",\n", 2);
            }

            o->write_characters(indent_string.c_str(), new_indent);
            o->write_character('\"');
            dump_escaped(i->first, ensure_ascii);
            o->write_characters("\": ", 3);
            dump(i->second, true, ensure_ascii, indent_step, new_indent);

            o->write_character('\n');
            o->write_characters(indent_string.c_str(), current_indent);
            o->write_character('}');
        } else {
            o->write_character('{');

            auto i = val.m_data.m_value.object->cbegin();
            for (size_t cnt = 0; cnt < val.m_data.m_value.object->size() - 1; ++cnt, ++i) {
                o->write_character('\"');
                dump_escaped(i->first, ensure_ascii);
                o->write_characters("\":", 2);
                dump(i->second, false, ensure_ascii, indent_step, current_indent);
                o->write_character(',');
            }

            o->write_character('\"');
            dump_escaped(i->first, ensure_ascii);
            o->write_characters("\":", 2);
            dump(i->second, false, ensure_ascii, indent_step, current_indent);

            o->write_character('}');
        }

        return;
    }

    case value_t::array: {
        if (val.m_data.m_value.array->empty()) {
            o->write_characters("[]", 2);
            return;
        }

        if (pretty_print) {
            o->write_characters("[\n", 2);

            const auto new_indent = current_indent + indent_step;
            if (indent_string.size() < new_indent) {
                indent_string.resize(indent_string.size() * 2, ' ');
            }

            for (auto i = val.m_data.m_value.array->cbegin();
                 i != val.m_data.m_value.array->cend() - 1; ++i) {
                o->write_characters(indent_string.c_str(), new_indent);
                dump(*i, true, ensure_ascii, indent_step, new_indent);
                o->write_characters(",\n", 2);
            }

            o->write_characters(indent_string.c_str(), new_indent);
            dump(val.m_data.m_value.array->back(), true, ensure_ascii, indent_step, new_indent);

            o->write_character('\n');
            o->write_characters(indent_string.c_str(), current_indent);
            o->write_character(']');
        } else {
            o->write_character('[');

            for (auto i = val.m_data.m_value.array->cbegin();
                 i != val.m_data.m_value.array->cend() - 1; ++i) {
                dump(*i, false, ensure_ascii, indent_step, current_indent);
                o->write_character(',');
            }

            dump(val.m_data.m_value.array->back(), false, ensure_ascii, indent_step,
                 current_indent);

            o->write_character(']');
        }

        return;
    }

    case value_t::string: {
        o->write_character('\"');
        dump_escaped(*val.m_data.m_value.string, ensure_ascii);
        o->write_character('\"');
        return;
    }

    case value_t::binary: {
        if (pretty_print) {
            o->write_characters("{\n", 2);

            const auto new_indent = current_indent + indent_step;
            if (indent_string.size() < new_indent) {
                indent_string.resize(indent_string.size() * 2, ' ');
            }

            o->write_characters(indent_string.c_str(), new_indent);

            o->write_characters("\"bytes\": [", 10);

            if (!val.m_data.m_value.binary->empty()) {
                for (auto i = val.m_data.m_value.binary->cbegin();
                     i != val.m_data.m_value.binary->cend() - 1; ++i) {
                    dump_integer(*i);
                    o->write_characters(", ", 2);
                }
                dump_integer(val.m_data.m_value.binary->back());
            }

            o->write_characters("],\n", 3);
            o->write_characters(indent_string.c_str(), new_indent);

            o->write_characters("\"subtype\": ", 11);
            if (val.m_data.m_value.binary->has_subtype()) {
                dump_integer(val.m_data.m_value.binary->subtype());
            } else {
                o->write_characters("null", 4);
            }
            o->write_character('\n');
            o->write_characters(indent_string.c_str(), current_indent);
            o->write_character('}');
        } else {
            o->write_characters("{\"bytes\":[", 10);

            if (!val.m_data.m_value.binary->empty()) {
                for (auto i = val.m_data.m_value.binary->cbegin();
                     i != val.m_data.m_value.binary->cend() - 1; ++i) {
                    dump_integer(*i);
                    o->write_character(',');
                }
                dump_integer(val.m_data.m_value.binary->back());
            }

            o->write_characters("],\"subtype\":", 12);
            if (val.m_data.m_value.binary->has_subtype()) {
                dump_integer(val.m_data.m_value.binary->subtype());
                o->write_character('}');
            } else {
                o->write_characters("null}", 5);
            }
        }
        return;
    }

    case value_t::boolean: {
        if (val.m_data.m_value.boolean) {
            o->write_characters("true", 4);
        } else {
            o->write_characters("false", 5);
        }
        return;
    }

    case value_t::number_integer: {
        dump_integer(val.m_data.m_value.number_integer);
        return;
    }

    case value_t::number_float: {
        dump_float(val.m_data.m_value.number_float);
        return;
    }

    case value_t::discarded: {
        o->write_characters("<discarded>", 11);
        return;
    }

    case value_t::null: {
        o->write_characters("null", 4);
        return;
    }

    default: return;
    }
}

void nlohmann::detail::serializer::dump_escaped(const std::string &s, const bool ensure_ascii)
{
    std::uint32_t codepoint{};
    uint8_t state = UTF8_ACCEPT;
    size_t bytes = 0;

    size_t bytes_after_last_accept = 0;
    size_t undumped_chars = 0;

    for (size_t i = 0; i < s.size(); ++i) {
        const auto byte = static_cast<uint8_t>(s[i]);

        switch (decode(state, codepoint, byte)) {
        case UTF8_ACCEPT: {
            switch (codepoint) {
            case 0x08: {
                string_buffer[bytes++] = '\\';
                string_buffer[bytes++] = 'b';
                break;
            }

            case 0x09: {
                string_buffer[bytes++] = '\\';
                string_buffer[bytes++] = 't';
                break;
            }

            case 0x0A: {
                string_buffer[bytes++] = '\\';
                string_buffer[bytes++] = 'n';
                break;
            }

            case 0x0C: {
                string_buffer[bytes++] = '\\';
                string_buffer[bytes++] = 'f';
                break;
            }

            case 0x0D: {
                string_buffer[bytes++] = '\\';
                string_buffer[bytes++] = 'r';
                break;
            }

            case 0x22: {
                string_buffer[bytes++] = '\\';
                string_buffer[bytes++] = '\"';
                break;
            }

            case 0x5C: {
                string_buffer[bytes++] = '\\';
                string_buffer[bytes++] = '\\';
                break;
            }

            default: {
                if ((codepoint <= 0x1F) || (ensure_ascii && (codepoint >= 0x7F))) {
                    if (codepoint <= 0xFFFF) {
                        static_cast<void>((std::snprintf)(string_buffer.data() + bytes, 7,
                                                          "\\u%04x",
                                                          static_cast<std::uint16_t>(codepoint)));
                        bytes += 6;
                    } else {
                        static_cast<void>((
                            std::snprintf)(string_buffer.data() + bytes, 13, "\\u%04x\\u%04x",
                                           static_cast<std::uint16_t>(0xD7C0u + (codepoint >> 10u)),
                                           static_cast<std::uint16_t>(0xDC00u
                                                                      + (codepoint & 0x3FFu))));
                        bytes += 12;
                    }
                } else {
                    string_buffer[bytes++] = s[i];
                }
                break;
            }
            }

            if (string_buffer.size() - bytes < 13) {
                o->write_characters(string_buffer.data(), bytes);
                bytes = 0;
            }

            bytes_after_last_accept = bytes;
            undumped_chars = 0;
            break;
        }

        case UTF8_REJECT: {
            switch (error_handler) {
            case error_handler_t::strict: {
                throw(type_error::create(316,
                                         concat("invalid UTF-8 byte at index ", std::to_string(i),
                                                ": 0x", hex_bytes(byte | 0))));
            }

            case error_handler_t::ignore:
            case error_handler_t::replace: {
                if (undumped_chars > 0) {
                    --i;
                }

                bytes = bytes_after_last_accept;

                if (error_handler == error_handler_t::replace) {
                    if (ensure_ascii) {
                        string_buffer[bytes++] = '\\';
                        string_buffer[bytes++] = 'u';
                        string_buffer[bytes++] = 'f';
                        string_buffer[bytes++] = 'f';
                        string_buffer[bytes++] = 'f';
                        string_buffer[bytes++] = 'd';
                    } else {
                        string_buffer[bytes++] = detail::binary_writer::to_char_type('\xEF');
                        string_buffer[bytes++] = detail::binary_writer::to_char_type('\xBF');
                        string_buffer[bytes++] = detail::binary_writer::to_char_type('\xBD');
                    }

                    if (string_buffer.size() - bytes < 13) {
                        o->write_characters(string_buffer.data(), bytes);
                        bytes = 0;
                    }

                    bytes_after_last_accept = bytes;
                }

                undumped_chars = 0;

                state = UTF8_ACCEPT;
                break;
            }

            default: return;
            }
            break;
        }

        default: {
            if (!ensure_ascii) {
                string_buffer[bytes++] = s[i];
            }
            ++undumped_chars;
            break;
        }
        }
    }

    if (state == UTF8_ACCEPT) {
        if (bytes > 0) {
            o->write_characters(string_buffer.data(), bytes);
        }
    } else {
        switch (error_handler) {
        case error_handler_t::strict: {
            throw(type_error::create(316, concat("incomplete UTF-8 string; last byte: 0x",
                                                 hex_bytes(static_cast<uint8_t>(s.back() | 0)))));
        }

        case error_handler_t::ignore: {
            o->write_characters(string_buffer.data(), bytes_after_last_accept);
            break;
        }

        case error_handler_t::replace: {
            o->write_characters(string_buffer.data(), bytes_after_last_accept);

            if (ensure_ascii) {
                o->write_characters("\\ufffd", 6);
            } else {
                o->write_characters("\xEF\xBF\xBD", 3);
            }
            break;
        }

        default: return;
        }
    }
}

unsigned int nlohmann::detail::serializer::count_digits(uint64_t x) noexcept
{
    unsigned int n_digits = 1;
    for (;;) {
        if (x < 10) {
            return n_digits;
        }
        if (x < 100) {
            return n_digits + 1;
        }
        if (x < 1000) {
            return n_digits + 2;
        }
        if (x < 10000) {
            return n_digits + 3;
        }
        x = x / 10000u;
        n_digits += 4;
    }
}

std::string nlohmann::detail::serializer::hex_bytes(uint8_t byte)
{
    std::string result = "FF";
    constexpr const char *nibble_to_hex = "0123456789ABCDEF";
    result[0] = nibble_to_hex[byte / 16];
    result[1] = nibble_to_hex[byte % 16];
    return result;
}

void nlohmann::detail::serializer::dump_integer(int64_t x)
{
    static constexpr std::array<std::array<char, 2>, 100> digits_to_99{{
        {{'0', '0'}}, {{'0', '1'}}, {{'0', '2'}}, {{'0', '3'}}, {{'0', '4'}}, {{'0', '5'}},
        {{'0', '6'}}, {{'0', '7'}}, {{'0', '8'}}, {{'0', '9'}}, {{'1', '0'}}, {{'1', '1'}},
        {{'1', '2'}}, {{'1', '3'}}, {{'1', '4'}}, {{'1', '5'}}, {{'1', '6'}}, {{'1', '7'}},
        {{'1', '8'}}, {{'1', '9'}}, {{'2', '0'}}, {{'2', '1'}}, {{'2', '2'}}, {{'2', '3'}},
        {{'2', '4'}}, {{'2', '5'}}, {{'2', '6'}}, {{'2', '7'}}, {{'2', '8'}}, {{'2', '9'}},
        {{'3', '0'}}, {{'3', '1'}}, {{'3', '2'}}, {{'3', '3'}}, {{'3', '4'}}, {{'3', '5'}},
        {{'3', '6'}}, {{'3', '7'}}, {{'3', '8'}}, {{'3', '9'}}, {{'4', '0'}}, {{'4', '1'}},
        {{'4', '2'}}, {{'4', '3'}}, {{'4', '4'}}, {{'4', '5'}}, {{'4', '6'}}, {{'4', '7'}},
        {{'4', '8'}}, {{'4', '9'}}, {{'5', '0'}}, {{'5', '1'}}, {{'5', '2'}}, {{'5', '3'}},
        {{'5', '4'}}, {{'5', '5'}}, {{'5', '6'}}, {{'5', '7'}}, {{'5', '8'}}, {{'5', '9'}},
        {{'6', '0'}}, {{'6', '1'}}, {{'6', '2'}}, {{'6', '3'}}, {{'6', '4'}}, {{'6', '5'}},
        {{'6', '6'}}, {{'6', '7'}}, {{'6', '8'}}, {{'6', '9'}}, {{'7', '0'}}, {{'7', '1'}},
        {{'7', '2'}}, {{'7', '3'}}, {{'7', '4'}}, {{'7', '5'}}, {{'7', '6'}}, {{'7', '7'}},
        {{'7', '8'}}, {{'7', '9'}}, {{'8', '0'}}, {{'8', '1'}}, {{'8', '2'}}, {{'8', '3'}},
        {{'8', '4'}}, {{'8', '5'}}, {{'8', '6'}}, {{'8', '7'}}, {{'8', '8'}}, {{'8', '9'}},
        {{'9', '0'}}, {{'9', '1'}}, {{'9', '2'}}, {{'9', '3'}}, {{'9', '4'}}, {{'9', '5'}},
        {{'9', '6'}}, {{'9', '7'}}, {{'9', '8'}}, {{'9', '9'}},
    }};

    if (x == 0) {
        o->write_character('0');
        return;
    }

    auto buffer_ptr = number_buffer.begin();

    uint64_t abs_value = 0;

    unsigned int n_chars{};

    if (x < 0) {
        *buffer_ptr = '-';
        abs_value = -x;

        n_chars = 1 + count_digits(abs_value);
    } else {
        abs_value = static_cast<uint64_t>(x);
        n_chars = count_digits(abs_value);
    }

    buffer_ptr += n_chars;

    while (abs_value >= 100) {
        const auto digits_index = static_cast<unsigned>((abs_value % 100));
        abs_value /= 100;
        *(--buffer_ptr) = digits_to_99[digits_index][1];
        *(--buffer_ptr) = digits_to_99[digits_index][0];
    }

    if (abs_value >= 10) {
        const auto digits_index = static_cast<unsigned>(abs_value);
        *(--buffer_ptr) = digits_to_99[digits_index][1];
        *(--buffer_ptr) = digits_to_99[digits_index][0];
    } else {
        *(--buffer_ptr) = static_cast<char>('0' + abs_value);
    }

    o->write_characters(number_buffer.data(), n_chars);
}

void nlohmann::detail::serializer::dump_float(double x)
{
    if (!std::isfinite(x)) {
        o->write_characters("null", 4);
        return;
    }

    static constexpr bool is_ieee_single_or_double
        = (std::numeric_limits<double>::is_iec559 && std::numeric_limits<double>::digits == 24
           && std::numeric_limits<double>::max_exponent == 128)
          || (std::numeric_limits<double>::is_iec559 && std::numeric_limits<double>::digits == 53
              && std::numeric_limits<double>::max_exponent == 1024);

    dump_float(x, std::integral_constant<bool, is_ieee_single_or_double>());
}

void nlohmann::detail::serializer::dump_float(double x, std::true_type)
{
    auto *begin = number_buffer.data();
    auto *end = ::nlohmann::detail::to_chars(begin, begin + number_buffer.size(), x);

    o->write_characters(begin, static_cast<size_t>(end - begin));
}

void nlohmann::detail::serializer::dump_float(double x, std::false_type)
{
    static constexpr auto d = std::numeric_limits<double>::max_digits10;

    std::ptrdiff_t len = (std::snprintf)(number_buffer.data(), number_buffer.size(), "%.*g", d, x);

    if (thousands_sep != '\0') {
        const auto end = std::remove(number_buffer.begin(), number_buffer.begin() + len,
                                     thousands_sep);
        std::fill(end, number_buffer.end(), '\0');

        len = (end - number_buffer.begin());
    }

    if (decimal_point != '\0' && decimal_point != '.') {
        const auto dec_pos = std::find(number_buffer.begin(), number_buffer.end(), decimal_point);
        if (dec_pos != number_buffer.end()) {
            *dec_pos = '.';
        }
    }

    o->write_characters(number_buffer.data(), static_cast<size_t>(len));

    const bool value_is_int_like = std::none_of(number_buffer.begin(),
                                                number_buffer.begin() + len + 1,
                                                [](char c) { return c == '.' || c == 'e'; });

    if (value_is_int_like) {
        o->write_characters(".0", 2);
    }
}

uint8_t nlohmann::detail::serializer::decode(uint8_t &state, uint32_t &codep,
                                             const uint8_t byte) noexcept
{
    static const std::array<uint8_t, 400> utf8d = {
        {0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
         0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
         0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
         0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
         0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
         0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
         0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
         0,   0,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,
         9,   9,   9,   9,   9,   9,   9,   9,   9,   9,   9,   9,   9,   9,   9,   9,   7,   7,
         7,   7,   7,   7,   7,   7,   7,   7,   7,   7,   7,   7,   7,   7,   7,   7,   7,   7,
         7,   7,   7,   7,   7,   7,   7,   7,   7,   7,   7,   7,   8,   8,   2,   2,   2,   2,
         2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,
         2,   2,   2,   2,   2,   2,   2,   2,   0xA, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3,
         0x3, 0x3, 0x3, 0x4, 0x3, 0x3, 0xB, 0x6, 0x6, 0x6, 0x5, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8,
         0x8, 0x8, 0x8, 0x8, 0x0, 0x1, 0x2, 0x3, 0x5, 0x8, 0x7, 0x1, 0x1, 0x1, 0x4, 0x6, 0x1, 0x1,
         0x1, 0x1, 1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,
         1,   0,   1,   1,   1,   1,   1,   0,   1,   0,   1,   1,   1,   1,   1,   1,   1,   2,
         1,   1,   1,   1,   1,   2,   1,   2,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,
         1,   1,   1,   2,   1,   1,   1,   1,   1,   1,   1,   1,   1,   2,   1,   1,   1,   1,
         1,   1,   1,   2,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   3,
         1,   3,   1,   1,   1,   1,   1,   1,   1,   3,   1,   1,   1,   1,   1,   3,   1,   3,
         1,   1,   1,   1,   1,   1,   1,   3,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,
         1,   1,   1,   1}};

    const uint8_t type = utf8d[byte];

    codep = (state != UTF8_ACCEPT) ? (byte & 0x3fu) | (codep << 6u) : (0xFFu >> type) & (byte);

    const size_t index = 256u + static_cast<size_t>(state) * 16u + static_cast<size_t>(type);

    state = utf8d[index];
    return state;
}
