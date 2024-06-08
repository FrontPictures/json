#include "binary_reader.hpp"
#include <cmath>
#include <iterator>
#include "nlohmann/byte_container_with_subtype.hpp"
#include "nlohmann/detail/string_concat.hpp"
#include "json_sax.hpp"
#include "lexer.hpp"
#include "input_adapters.hpp"

nlohmann::detail::binary_reader::binary_reader(const iterator_input_adapter &adapter,
                                               const input_format_t format) noexcept
    : ia(new iterator_input_adapter(adapter)), input_format(format)
{}

nlohmann::detail::binary_reader::~binary_reader() = default;

bool nlohmann::detail::binary_reader::sax_parse(const input_format_t format, json_sax *sax_,
                                                const bool strict,
                                                const cbor_tag_handler_t tag_handler)
{
    sax = sax_;
    bool result = false;

    switch (format) {
    case input_format_t::bson: result = parse_bson_internal(); break;

    case input_format_t::cbor: result = parse_cbor_internal(true, tag_handler); break;

    case input_format_t::msgpack: result = parse_msgpack_internal(); break;

    case input_format_t::ubjson:
    case input_format_t::bjdata: result = parse_ubjson_internal(); break;

    case input_format_t::json:
    default: return false;
    }

    if (result && strict) {
        if (input_format == input_format_t::ubjson || input_format == input_format_t::bjdata) {
            get_ignore_noop();
        } else {
            get();
        }

        if (current != uint64_t(EOF)) {
            return sax
                ->parse_error(chars_read, get_token_string(),
                              parse_error::create(
                                  110, chars_read,
                                  exception_message(input_format,
                                                    concat("expected end of input; last byte: 0x",
                                                           get_token_string()),
                                                    "value")));
        }
    }

    return result;
}

bool nlohmann::detail::binary_reader::parse_bson_internal()
{
    int32_t document_size{};
    get_number<int32_t, true>(input_format_t::bson, document_size);

    if (!sax->start_object(static_cast<size_t>(-1))) {
        return false;
    }

    if (!parse_bson_element_list(/*is_array*/ false)) {
        return false;
    }

    return sax->end_object();
}

bool nlohmann::detail::binary_reader::get_bson_cstr(std::string &result)
{
    auto out = std::back_inserter(result);
    while (true) {
        get();
        if (!unexpect_eof(input_format_t::bson, "cstring")) {
            return false;
        }
        if (current == 0x00) {
            return true;
        }
        *out++ = static_cast<typename std::string::value_type>(current);
    }
}

bool nlohmann::detail::binary_reader::get_bson_string(const int32_t len, std::string &result)
{
    if (len < 1) {
        auto last_token = get_token_string();
        return sax->parse_error(
            chars_read, last_token,
            parse_error::create(112, chars_read,
                                exception_message(input_format_t::bson,
                                                  concat("string length must be at least 1, is ",
                                                         std::to_string(len)),
                                                  "string")));
    }

    return get_string(input_format_t::bson, len - 1, result) && get() != uint64_t(EOF);
}

bool nlohmann::detail::binary_reader::get_bson_binary(const int32_t len,
                                                      byte_container_with_subtype &result)
{
    if (len < 0) {
        auto last_token = get_token_string();
        return sax
            ->parse_error(chars_read, last_token,
                          parse_error::create(
                              112, chars_read,
                              exception_message(input_format_t::bson,
                                                concat("byte array length cannot be negative, is ",
                                                       std::to_string(len)),
                                                "binary")));
    }

    uint8_t subtype{};
    get_number<uint8_t>(input_format_t::bson, subtype);
    result.set_subtype(subtype);

    return get_binary(input_format_t::bson, len, result);
}

bool nlohmann::detail::binary_reader::parse_bson_element_internal(
    const uint64_t element_type, const size_t element_type_parse_position)
{
    switch (element_type) {
    case 0x01: {
        double number{};
        return get_number<double, true>(input_format_t::bson, number)
               && sax->number_float(static_cast<double>(number), "");
    }

    case 0x02: {
        int32_t len{};
        std::string value;
        return get_number<int32_t, true>(input_format_t::bson, len) && get_bson_string(len, value)
               && sax->string(value);
    }

    case 0x03: {
        return parse_bson_internal();
    }

    case 0x04: {
        return parse_bson_array();
    }

    case 0x05: {
        int32_t len{};
        byte_container_with_subtype value;
        return get_number<int32_t, true>(input_format_t::bson, len) && get_bson_binary(len, value)
               && sax->binary(value);
    }

    case 0x08: {
        return sax->boolean(get() != 0);
    }

    case 0x0A: {
        return sax->null();
    }

    case 0x10: {
        int32_t value{};
        return get_number<int32_t, true>(input_format_t::bson, value)
               && sax->number_integer(value);
    }

    case 0x12: {
        int64_t value{};
        return get_number<int64_t, true>(input_format_t::bson, value)
               && sax->number_integer(value);
    }

    default: {
        std::array<char, 3> cr{{}};
        static_cast<void>((std::snprintf)(cr.data(), cr.size(), "%.2hhX",
                                          static_cast<unsigned char>(element_type)));
        const std::string cr_str{cr.data()};
        return sax->parse_error(element_type_parse_position, cr_str,
                                parse_error::create(114, element_type_parse_position,
                                                    concat("Unsupported BSON record type 0x",
                                                           cr_str)));
    }
    }
}

bool nlohmann::detail::binary_reader::parse_bson_element_list(const bool is_array)
{
    std::string key;

    while (auto element_type = get()) {
        if (!unexpect_eof(input_format_t::bson, "element list")) {
            return false;
        }

        const size_t element_type_parse_position = chars_read;
        if (!get_bson_cstr(key)) {
            return false;
        }

        if (!is_array && !sax->key(key)) {
            return false;
        }

        if (!parse_bson_element_internal(element_type, element_type_parse_position)) {
            return false;
        }

        key.clear();
    }

    return true;
}

bool nlohmann::detail::binary_reader::parse_bson_array()
{
    int32_t document_size{};
    get_number<int32_t, true>(input_format_t::bson, document_size);

    if (!sax->start_array(static_cast<size_t>(-1))) {
        return false;
    }

    if (!parse_bson_element_list(/*is_array*/ true)) {
        return false;
    }

    return sax->end_array();
}

bool nlohmann::detail::binary_reader::parse_cbor_internal(const bool get_char,
                                                          const cbor_tag_handler_t tag_handler)
{
    switch (get_char ? get() : current) {
    case uint64_t(EOF): return unexpect_eof(input_format_t::cbor, "value");

    case 0x00:
    case 0x01:
    case 0x02:
    case 0x03:
    case 0x04:
    case 0x05:
    case 0x06:
    case 0x07:
    case 0x08:
    case 0x09:
    case 0x0A:
    case 0x0B:
    case 0x0C:
    case 0x0D:
    case 0x0E:
    case 0x0F:
    case 0x10:
    case 0x11:
    case 0x12:
    case 0x13:
    case 0x14:
    case 0x15:
    case 0x16:
    case 0x17: return sax->number_integer(current);

    case 0x18: {
        uint8_t number{};
        return get_number(input_format_t::cbor, number) && sax->number_integer(number);
    }

    case 0x19: {
        std::uint16_t number{};
        return get_number(input_format_t::cbor, number) && sax->number_integer(number);
    }

    case 0x1A: {
        std::uint32_t number{};
        return get_number(input_format_t::cbor, number) && sax->number_integer(number);
    }

    case 0x1B: {
        int64_t number{};
        return get_number(input_format_t::cbor, number) && sax->number_integer(number);
    }

    case 0x20:
    case 0x21:
    case 0x22:
    case 0x23:
    case 0x24:
    case 0x25:
    case 0x26:
    case 0x27:
    case 0x28:
    case 0x29:
    case 0x2A:
    case 0x2B:
    case 0x2C:
    case 0x2D:
    case 0x2E:
    case 0x2F:
    case 0x30:
    case 0x31:
    case 0x32:
    case 0x33:
    case 0x34:
    case 0x35:
    case 0x36:
    case 0x37: return sax->number_integer(static_cast<std::int8_t>(0x20 - 1 - current));

    case 0x38: {
        uint8_t number{};
        return get_number(input_format_t::cbor, number)
               && sax->number_integer(static_cast<int64_t>(-1) - number);
    }

    case 0x39: {
        std::uint16_t number{};
        return get_number(input_format_t::cbor, number)
               && sax->number_integer(static_cast<int64_t>(-1) - number);
    }

    case 0x3A: {
        std::uint32_t number{};
        return get_number(input_format_t::cbor, number)
               && sax->number_integer(static_cast<int64_t>(-1) - number);
    }

    case 0x3B: {
        uint64_t number{};
        return get_number(input_format_t::cbor, number)
               && sax->number_integer(static_cast<int64_t>(-1) - static_cast<int64_t>(number));
    }

    case 0x40:
    case 0x41:
    case 0x42:
    case 0x43:
    case 0x44:
    case 0x45:
    case 0x46:
    case 0x47:
    case 0x48:
    case 0x49:
    case 0x4A:
    case 0x4B:
    case 0x4C:
    case 0x4D:
    case 0x4E:
    case 0x4F:
    case 0x50:
    case 0x51:
    case 0x52:
    case 0x53:
    case 0x54:
    case 0x55:
    case 0x56:
    case 0x57:
    case 0x58:
    case 0x59:
    case 0x5A:
    case 0x5B:
    case 0x5F: {
        byte_container_with_subtype b;
        return get_cbor_binary(b) && sax->binary(b);
    }

    case 0x60:
    case 0x61:
    case 0x62:
    case 0x63:
    case 0x64:
    case 0x65:
    case 0x66:
    case 0x67:
    case 0x68:
    case 0x69:
    case 0x6A:
    case 0x6B:
    case 0x6C:
    case 0x6D:
    case 0x6E:
    case 0x6F:
    case 0x70:
    case 0x71:
    case 0x72:
    case 0x73:
    case 0x74:
    case 0x75:
    case 0x76:
    case 0x77:
    case 0x78:
    case 0x79:
    case 0x7A:
    case 0x7B:
    case 0x7F: {
        std::string s;
        return get_cbor_string(s) && sax->string(s);
    }

    case 0x80:
    case 0x81:
    case 0x82:
    case 0x83:
    case 0x84:
    case 0x85:
    case 0x86:
    case 0x87:
    case 0x88:
    case 0x89:
    case 0x8A:
    case 0x8B:
    case 0x8C:
    case 0x8D:
    case 0x8E:
    case 0x8F:
    case 0x90:
    case 0x91:
    case 0x92:
    case 0x93:
    case 0x94:
    case 0x95:
    case 0x96:
    case 0x97:
        return get_cbor_array(static_cast<size_t>(static_cast<unsigned int>(current) & 0x1Fu),
                              tag_handler);

    case 0x98: {
        uint8_t len{};
        return get_number(input_format_t::cbor, len)
               && get_cbor_array(static_cast<size_t>(len), tag_handler);
    }

    case 0x99: {
        std::uint16_t len{};
        return get_number(input_format_t::cbor, len)
               && get_cbor_array(static_cast<size_t>(len), tag_handler);
    }

    case 0x9A: {
        std::uint32_t len{};
        return get_number(input_format_t::cbor, len)
               && get_cbor_array(static_cast<size_t>(len), tag_handler);
    }

    case 0x9B: {
        uint64_t len{};
        return get_number(input_format_t::cbor, len)
               && get_cbor_array(static_cast<size_t>(len), tag_handler);
    }

    case 0x9F: return get_cbor_array(static_cast<size_t>(-1), tag_handler);

    case 0xA0:
    case 0xA1:
    case 0xA2:
    case 0xA3:
    case 0xA4:
    case 0xA5:
    case 0xA6:
    case 0xA7:
    case 0xA8:
    case 0xA9:
    case 0xAA:
    case 0xAB:
    case 0xAC:
    case 0xAD:
    case 0xAE:
    case 0xAF:
    case 0xB0:
    case 0xB1:
    case 0xB2:
    case 0xB3:
    case 0xB4:
    case 0xB5:
    case 0xB6:
    case 0xB7:
        return get_cbor_object(static_cast<size_t>(static_cast<unsigned int>(current) & 0x1Fu),
                               tag_handler);

    case 0xB8: {
        uint8_t len{};
        return get_number(input_format_t::cbor, len)
               && get_cbor_object(static_cast<size_t>(len), tag_handler);
    }

    case 0xB9: {
        std::uint16_t len{};
        return get_number(input_format_t::cbor, len)
               && get_cbor_object(static_cast<size_t>(len), tag_handler);
    }

    case 0xBA: {
        std::uint32_t len{};
        return get_number(input_format_t::cbor, len)
               && get_cbor_object(static_cast<size_t>(len), tag_handler);
    }

    case 0xBB: {
        uint64_t len{};
        return get_number(input_format_t::cbor, len)
               && get_cbor_object(static_cast<size_t>(len), tag_handler);
    }

    case 0xBF: return get_cbor_object(static_cast<size_t>(-1), tag_handler);

    case 0xC6:
    case 0xC7:
    case 0xC8:
    case 0xC9:
    case 0xCA:
    case 0xCB:
    case 0xCC:
    case 0xCD:
    case 0xCE:
    case 0xCF:
    case 0xD0:
    case 0xD1:
    case 0xD2:
    case 0xD3:
    case 0xD4:
    case 0xD8:
    case 0xD9:
    case 0xDA:
    case 0xDB: {
        switch (tag_handler) {
        case cbor_tag_handler_t::error: {
            auto last_token = get_token_string();
            return sax
                ->parse_error(chars_read, last_token,
                              parse_error::create(112, chars_read,
                                                  exception_message(input_format_t::cbor,
                                                                    concat("invalid byte: 0x",
                                                                           last_token),
                                                                    "value")));
        }

        case cbor_tag_handler_t::ignore: {
            switch (current) {
            case 0xD8: {
                uint8_t subtype_to_ignore{};
                get_number(input_format_t::cbor, subtype_to_ignore);
                break;
            }
            case 0xD9: {
                std::uint16_t subtype_to_ignore{};
                get_number(input_format_t::cbor, subtype_to_ignore);
                break;
            }
            case 0xDA: {
                std::uint32_t subtype_to_ignore{};
                get_number(input_format_t::cbor, subtype_to_ignore);
                break;
            }
            case 0xDB: {
                uint64_t subtype_to_ignore{};
                get_number(input_format_t::cbor, subtype_to_ignore);
                break;
            }
            default: break;
            }
            return parse_cbor_internal(true, tag_handler);
        }

        case cbor_tag_handler_t::store: {
            byte_container_with_subtype b;

            switch (current) {
            case 0xD8: {
                uint8_t subtype{};
                get_number(input_format_t::cbor, subtype);
                b.set_subtype(
                    static_cast<typename byte_container_with_subtype::subtype_type>(subtype));
                break;
            }
            case 0xD9: {
                std::uint16_t subtype{};
                get_number(input_format_t::cbor, subtype);
                b.set_subtype(
                    static_cast<typename byte_container_with_subtype::subtype_type>(subtype));
                break;
            }
            case 0xDA: {
                std::uint32_t subtype{};
                get_number(input_format_t::cbor, subtype);
                b.set_subtype(
                    static_cast<typename byte_container_with_subtype::subtype_type>(subtype));
                break;
            }
            case 0xDB: {
                uint64_t subtype{};
                get_number(input_format_t::cbor, subtype);
                b.set_subtype(
                    static_cast<typename byte_container_with_subtype::subtype_type>(subtype));
                break;
            }
            default: return parse_cbor_internal(true, tag_handler);
            }
            get();
            return get_cbor_binary(b) && sax->binary(b);
        }

        default: return false;
        }
    }

    case 0xF4: return sax->boolean(false);

    case 0xF5: return sax->boolean(true);

    case 0xF6: return sax->null();

    case 0xF9: {
        const auto byte1_raw = get();
        if (!unexpect_eof(input_format_t::cbor, "number")) {
            return false;
        }
        const auto byte2_raw = get();
        if (!unexpect_eof(input_format_t::cbor, "number")) {
            return false;
        }

        const auto byte1 = static_cast<unsigned char>(byte1_raw);
        const auto byte2 = static_cast<unsigned char>(byte2_raw);

        const auto half = static_cast<unsigned int>((byte1 << 8u) + byte2);
        const double val = [&half] {
            const int exp = (half >> 10u) & 0x1Fu;
            const unsigned int mant = half & 0x3FFu;

            switch (exp) {
            case 0: return std::ldexp(mant, -24);
            case 31:
                return (mant == 0) ? std::numeric_limits<double>::infinity()
                                   : std::numeric_limits<double>::quiet_NaN();
            default: return std::ldexp(mant + 1024, exp - 25);
            }
        }();
        return sax->number_float((half & 0x8000u) != 0 ? static_cast<double>(-val)
                                                       : static_cast<double>(val),
                                 "");
    }

    case 0xFA: {
        float number{};
        return get_number(input_format_t::cbor, number)
               && sax->number_float(static_cast<double>(number), "");
    }

    case 0xFB: {
        double number{};
        return get_number(input_format_t::cbor, number)
               && sax->number_float(static_cast<double>(number), "");
    }

    default: {
        auto last_token = get_token_string();
        return sax->parse_error(chars_read, last_token,
                                parse_error::create(112, chars_read,
                                                    exception_message(input_format_t::cbor,
                                                                      concat("invalid byte: 0x",
                                                                             last_token),
                                                                      "value")));
    }
    }
}

bool nlohmann::detail::binary_reader::get_cbor_string(std::string &result)
{
    if (!unexpect_eof(input_format_t::cbor, "string")) {
        return false;
    }

    switch (current) {
    case 0x60:
    case 0x61:
    case 0x62:
    case 0x63:
    case 0x64:
    case 0x65:
    case 0x66:
    case 0x67:
    case 0x68:
    case 0x69:
    case 0x6A:
    case 0x6B:
    case 0x6C:
    case 0x6D:
    case 0x6E:
    case 0x6F:
    case 0x70:
    case 0x71:
    case 0x72:
    case 0x73:
    case 0x74:
    case 0x75:
    case 0x76:
    case 0x77: {
        return get_string(input_format_t::cbor, static_cast<unsigned int>(current) & 0x1Fu, result);
    }

    case 0x78: {
        uint8_t len{};
        return get_number(input_format_t::cbor, len)
               && get_string(input_format_t::cbor, len, result);
    }

    case 0x79: {
        std::uint16_t len{};
        return get_number(input_format_t::cbor, len)
               && get_string(input_format_t::cbor, len, result);
    }

    case 0x7A: {
        std::uint32_t len{};
        return get_number(input_format_t::cbor, len)
               && get_string(input_format_t::cbor, len, result);
    }

    case 0x7B: {
        uint64_t len{};
        return get_number(input_format_t::cbor, len)
               && get_string(input_format_t::cbor, len, result);
    }

    case 0x7F: {
        while (get() != 0xFF) {
            std::string chunk;
            if (!get_cbor_string(chunk)) {
                return false;
            }
            result.append(chunk);
        }
        return true;
    }

    default: {
        auto last_token = get_token_string();
        return sax->parse_error(
            chars_read, last_token,
            parse_error::create(
                113, chars_read,
                exception_message(input_format_t::cbor,
                                  concat("expected length specification (0x60-0x7B) or "
                                         "indefinite string type (0x7F); last byte: 0x",
                                         last_token),
                                  "string")));
    }
    }
}

bool nlohmann::detail::binary_reader::get_cbor_binary(byte_container_with_subtype &result)
{
    if (!unexpect_eof(input_format_t::cbor, "binary")) {
        return false;
    }

    switch (current) {
    case 0x40:
    case 0x41:
    case 0x42:
    case 0x43:
    case 0x44:
    case 0x45:
    case 0x46:
    case 0x47:
    case 0x48:
    case 0x49:
    case 0x4A:
    case 0x4B:
    case 0x4C:
    case 0x4D:
    case 0x4E:
    case 0x4F:
    case 0x50:
    case 0x51:
    case 0x52:
    case 0x53:
    case 0x54:
    case 0x55:
    case 0x56:
    case 0x57: {
        return get_binary(input_format_t::cbor, static_cast<unsigned int>(current) & 0x1Fu, result);
    }

    case 0x58: {
        uint8_t len{};
        return get_number(input_format_t::cbor, len)
               && get_binary(input_format_t::cbor, len, result);
    }

    case 0x59: {
        std::uint16_t len{};
        return get_number(input_format_t::cbor, len)
               && get_binary(input_format_t::cbor, len, result);
    }

    case 0x5A: {
        std::uint32_t len{};
        return get_number(input_format_t::cbor, len)
               && get_binary(input_format_t::cbor, len, result);
    }

    case 0x5B: {
        uint64_t len{};
        return get_number(input_format_t::cbor, len)
               && get_binary(input_format_t::cbor, len, result);
    }

    case 0x5F: {
        while (get() != 0xFF) {
            byte_container_with_subtype chunk;
            if (!get_cbor_binary(chunk)) {
                return false;
            }
            result.insert(result.end(), chunk.begin(), chunk.end());
        }
        return true;
    }

    default: {
        auto last_token = get_token_string();
        return sax->parse_error(
            chars_read, last_token,
            parse_error::create(
                113, chars_read,
                exception_message(input_format_t::cbor,
                                  concat("expected length specification (0x40-0x5B) or "
                                         "indefinite binary array type (0x5F); last byte: 0x",
                                         last_token),
                                  "binary")));
    }
    }
}

bool nlohmann::detail::binary_reader::get_cbor_array(const size_t len,
                                                     const cbor_tag_handler_t tag_handler)
{
    if (!sax->start_array(len)) {
        return false;
    }

    if (len != static_cast<size_t>(-1)) {
        for (size_t i = 0; i < len; ++i) {
            if (!parse_cbor_internal(true, tag_handler)) {
                return false;
            }
        }
    } else {
        while (get() != 0xFF) {
            if (!parse_cbor_internal(false, tag_handler)) {
                return false;
            }
        }
    }

    return sax->end_array();
}

bool nlohmann::detail::binary_reader::get_cbor_object(const size_t len,
                                                      const cbor_tag_handler_t tag_handler)
{
    if (!sax->start_object(len)) {
        return false;
    }

    if (len != 0) {
        std::string key;
        if (len != static_cast<size_t>(-1)) {
            for (size_t i = 0; i < len; ++i) {
                get();
                if (!get_cbor_string(key) || !sax->key(key)) {
                    return false;
                }

                if (!parse_cbor_internal(true, tag_handler)) {
                    return false;
                }
                key.clear();
            }
        } else {
            while (get() != 0xFF) {
                if (!get_cbor_string(key) || !sax->key(key)) {
                    return false;
                }

                if (!parse_cbor_internal(true, tag_handler)) {
                    return false;
                }
                key.clear();
            }
        }
    }

    return sax->end_object();
}

bool nlohmann::detail::binary_reader::parse_msgpack_internal()
{
    switch (get()) {
    case uint64_t(EOF): return unexpect_eof(input_format_t::msgpack, "value");

    case 0x00:
    case 0x01:
    case 0x02:
    case 0x03:
    case 0x04:
    case 0x05:
    case 0x06:
    case 0x07:
    case 0x08:
    case 0x09:
    case 0x0A:
    case 0x0B:
    case 0x0C:
    case 0x0D:
    case 0x0E:
    case 0x0F:
    case 0x10:
    case 0x11:
    case 0x12:
    case 0x13:
    case 0x14:
    case 0x15:
    case 0x16:
    case 0x17:
    case 0x18:
    case 0x19:
    case 0x1A:
    case 0x1B:
    case 0x1C:
    case 0x1D:
    case 0x1E:
    case 0x1F:
    case 0x20:
    case 0x21:
    case 0x22:
    case 0x23:
    case 0x24:
    case 0x25:
    case 0x26:
    case 0x27:
    case 0x28:
    case 0x29:
    case 0x2A:
    case 0x2B:
    case 0x2C:
    case 0x2D:
    case 0x2E:
    case 0x2F:
    case 0x30:
    case 0x31:
    case 0x32:
    case 0x33:
    case 0x34:
    case 0x35:
    case 0x36:
    case 0x37:
    case 0x38:
    case 0x39:
    case 0x3A:
    case 0x3B:
    case 0x3C:
    case 0x3D:
    case 0x3E:
    case 0x3F:
    case 0x40:
    case 0x41:
    case 0x42:
    case 0x43:
    case 0x44:
    case 0x45:
    case 0x46:
    case 0x47:
    case 0x48:
    case 0x49:
    case 0x4A:
    case 0x4B:
    case 0x4C:
    case 0x4D:
    case 0x4E:
    case 0x4F:
    case 0x50:
    case 0x51:
    case 0x52:
    case 0x53:
    case 0x54:
    case 0x55:
    case 0x56:
    case 0x57:
    case 0x58:
    case 0x59:
    case 0x5A:
    case 0x5B:
    case 0x5C:
    case 0x5D:
    case 0x5E:
    case 0x5F:
    case 0x60:
    case 0x61:
    case 0x62:
    case 0x63:
    case 0x64:
    case 0x65:
    case 0x66:
    case 0x67:
    case 0x68:
    case 0x69:
    case 0x6A:
    case 0x6B:
    case 0x6C:
    case 0x6D:
    case 0x6E:
    case 0x6F:
    case 0x70:
    case 0x71:
    case 0x72:
    case 0x73:
    case 0x74:
    case 0x75:
    case 0x76:
    case 0x77:
    case 0x78:
    case 0x79:
    case 0x7A:
    case 0x7B:
    case 0x7C:
    case 0x7D:
    case 0x7E:
    case 0x7F: return sax->number_integer(current);

    case 0x80:
    case 0x81:
    case 0x82:
    case 0x83:
    case 0x84:
    case 0x85:
    case 0x86:
    case 0x87:
    case 0x88:
    case 0x89:
    case 0x8A:
    case 0x8B:
    case 0x8C:
    case 0x8D:
    case 0x8E:
    case 0x8F:
        return get_msgpack_object(static_cast<size_t>(static_cast<unsigned int>(current) & 0x0Fu));

    case 0x90:
    case 0x91:
    case 0x92:
    case 0x93:
    case 0x94:
    case 0x95:
    case 0x96:
    case 0x97:
    case 0x98:
    case 0x99:
    case 0x9A:
    case 0x9B:
    case 0x9C:
    case 0x9D:
    case 0x9E:
    case 0x9F:
        return get_msgpack_array(static_cast<size_t>(static_cast<unsigned int>(current) & 0x0Fu));

    case 0xA0:
    case 0xA1:
    case 0xA2:
    case 0xA3:
    case 0xA4:
    case 0xA5:
    case 0xA6:
    case 0xA7:
    case 0xA8:
    case 0xA9:
    case 0xAA:
    case 0xAB:
    case 0xAC:
    case 0xAD:
    case 0xAE:
    case 0xAF:
    case 0xB0:
    case 0xB1:
    case 0xB2:
    case 0xB3:
    case 0xB4:
    case 0xB5:
    case 0xB6:
    case 0xB7:
    case 0xB8:
    case 0xB9:
    case 0xBA:
    case 0xBB:
    case 0xBC:
    case 0xBD:
    case 0xBE:
    case 0xBF:
    case 0xD9:
    case 0xDA:
    case 0xDB: {
        std::string s;
        return get_msgpack_string(s) && sax->string(s);
    }

    case 0xC0: return sax->null();

    case 0xC2: return sax->boolean(false);

    case 0xC3: return sax->boolean(true);

    case 0xC4:
    case 0xC5:
    case 0xC6:
    case 0xC7:
    case 0xC8:
    case 0xC9:
    case 0xD4:
    case 0xD5:
    case 0xD6:
    case 0xD7:
    case 0xD8: {
        byte_container_with_subtype b;
        return get_msgpack_binary(b) && sax->binary(b);
    }

    case 0xCA: {
        float number{};
        return get_number(input_format_t::msgpack, number)
               && sax->number_float(static_cast<double>(number), "");
    }

    case 0xCB: {
        double number{};
        return get_number(input_format_t::msgpack, number)
               && sax->number_float(static_cast<double>(number), "");
    }

    case 0xCC: {
        uint8_t number{};
        return get_number(input_format_t::msgpack, number) && sax->number_integer(number);
    }

    case 0xCD: {
        std::uint16_t number{};
        return get_number(input_format_t::msgpack, number) && sax->number_integer(number);
    }

    case 0xCE: {
        std::uint32_t number{};
        return get_number(input_format_t::msgpack, number) && sax->number_integer(number);
    }

    case 0xCF: {
        int64_t number{};
        return get_number(input_format_t::msgpack, number) && sax->number_integer(number);
    }

    case 0xD0: {
        std::int8_t number{};
        return get_number(input_format_t::msgpack, number) && sax->number_integer(number);
    }

    case 0xD1: {
        std::int16_t number{};
        return get_number(input_format_t::msgpack, number) && sax->number_integer(number);
    }

    case 0xD2: {
        int32_t number{};
        return get_number(input_format_t::msgpack, number) && sax->number_integer(number);
    }

    case 0xD3: {
        int64_t number{};
        return get_number(input_format_t::msgpack, number) && sax->number_integer(number);
    }

    case 0xDC: {
        std::uint16_t len{};
        return get_number(input_format_t::msgpack, len)
               && get_msgpack_array(static_cast<size_t>(len));
    }

    case 0xDD: {
        std::uint32_t len{};
        return get_number(input_format_t::msgpack, len)
               && get_msgpack_array(static_cast<size_t>(len));
    }

    case 0xDE: {
        std::uint16_t len{};
        return get_number(input_format_t::msgpack, len)
               && get_msgpack_object(static_cast<size_t>(len));
    }

    case 0xDF: {
        std::uint32_t len{};
        return get_number(input_format_t::msgpack, len)
               && get_msgpack_object(static_cast<size_t>(len));
    }

    case 0xE0:
    case 0xE1:
    case 0xE2:
    case 0xE3:
    case 0xE4:
    case 0xE5:
    case 0xE6:
    case 0xE7:
    case 0xE8:
    case 0xE9:
    case 0xEA:
    case 0xEB:
    case 0xEC:
    case 0xED:
    case 0xEE:
    case 0xEF:
    case 0xF0:
    case 0xF1:
    case 0xF2:
    case 0xF3:
    case 0xF4:
    case 0xF5:
    case 0xF6:
    case 0xF7:
    case 0xF8:
    case 0xF9:
    case 0xFA:
    case 0xFB:
    case 0xFC:
    case 0xFD:
    case 0xFE:
    case 0xFF: return sax->number_integer(static_cast<std::int8_t>(current));

    default: {
        auto last_token = get_token_string();
        return sax->parse_error(chars_read, last_token,
                                parse_error::create(112, chars_read,
                                                    exception_message(input_format_t::msgpack,
                                                                      concat("invalid byte: 0x",
                                                                             last_token),
                                                                      "value")));
    }
    }
}

bool nlohmann::detail::binary_reader::get_msgpack_string(std::string &result)
{
    if (!unexpect_eof(input_format_t::msgpack, "string")) {
        return false;
    }

    switch (current) {
    case 0xA0:
    case 0xA1:
    case 0xA2:
    case 0xA3:
    case 0xA4:
    case 0xA5:
    case 0xA6:
    case 0xA7:
    case 0xA8:
    case 0xA9:
    case 0xAA:
    case 0xAB:
    case 0xAC:
    case 0xAD:
    case 0xAE:
    case 0xAF:
    case 0xB0:
    case 0xB1:
    case 0xB2:
    case 0xB3:
    case 0xB4:
    case 0xB5:
    case 0xB6:
    case 0xB7:
    case 0xB8:
    case 0xB9:
    case 0xBA:
    case 0xBB:
    case 0xBC:
    case 0xBD:
    case 0xBE:
    case 0xBF: {
        return get_string(input_format_t::msgpack, static_cast<unsigned int>(current) & 0x1Fu,
                          result);
    }

    case 0xD9: {
        uint8_t len{};
        return get_number(input_format_t::msgpack, len)
               && get_string(input_format_t::msgpack, len, result);
    }

    case 0xDA: {
        std::uint16_t len{};
        return get_number(input_format_t::msgpack, len)
               && get_string(input_format_t::msgpack, len, result);
    }

    case 0xDB: {
        std::uint32_t len{};
        return get_number(input_format_t::msgpack, len)
               && get_string(input_format_t::msgpack, len, result);
    }

    default: {
        auto last_token = get_token_string();
        return sax->parse_error(
            chars_read, last_token,
            parse_error::create(113, chars_read,
                                exception_message(input_format_t::msgpack,
                                                  concat("expected length specification "
                                                         "(0xA0-0xBF, 0xD9-0xDB); last byte: 0x",
                                                         last_token),
                                                  "string")));
    }
    }
}

bool nlohmann::detail::binary_reader::get_msgpack_binary(byte_container_with_subtype &result)
{
    auto assign_and_return_true = [&result](std::int8_t subtype) {
        result.set_subtype(static_cast<uint8_t>(subtype));
        return true;
    };

    switch (current) {
    case 0xC4: {
        uint8_t len{};
        return get_number(input_format_t::msgpack, len)
               && get_binary(input_format_t::msgpack, len, result);
    }

    case 0xC5: {
        std::uint16_t len{};
        return get_number(input_format_t::msgpack, len)
               && get_binary(input_format_t::msgpack, len, result);
    }

    case 0xC6: {
        std::uint32_t len{};
        return get_number(input_format_t::msgpack, len)
               && get_binary(input_format_t::msgpack, len, result);
    }

    case 0xC7: {
        uint8_t len{};
        std::int8_t subtype{};
        return get_number(input_format_t::msgpack, len)
               && get_number(input_format_t::msgpack, subtype)
               && get_binary(input_format_t::msgpack, len, result)
               && assign_and_return_true(subtype);
    }

    case 0xC8: {
        std::uint16_t len{};
        std::int8_t subtype{};
        return get_number(input_format_t::msgpack, len)
               && get_number(input_format_t::msgpack, subtype)
               && get_binary(input_format_t::msgpack, len, result)
               && assign_and_return_true(subtype);
    }

    case 0xC9: {
        std::uint32_t len{};
        std::int8_t subtype{};
        return get_number(input_format_t::msgpack, len)
               && get_number(input_format_t::msgpack, subtype)
               && get_binary(input_format_t::msgpack, len, result)
               && assign_and_return_true(subtype);
    }

    case 0xD4: {
        std::int8_t subtype{};
        return get_number(input_format_t::msgpack, subtype)
               && get_binary(input_format_t::msgpack, 1, result)
               && assign_and_return_true(subtype);
    }

    case 0xD5: {
        std::int8_t subtype{};
        return get_number(input_format_t::msgpack, subtype)
               && get_binary(input_format_t::msgpack, 2, result)
               && assign_and_return_true(subtype);
    }

    case 0xD6: {
        std::int8_t subtype{};
        return get_number(input_format_t::msgpack, subtype)
               && get_binary(input_format_t::msgpack, 4, result)
               && assign_and_return_true(subtype);
    }

    case 0xD7: {
        std::int8_t subtype{};
        return get_number(input_format_t::msgpack, subtype)
               && get_binary(input_format_t::msgpack, 8, result)
               && assign_and_return_true(subtype);
    }

    case 0xD8: {
        std::int8_t subtype{};
        return get_number(input_format_t::msgpack, subtype)
               && get_binary(input_format_t::msgpack, 16, result)
               && assign_and_return_true(subtype);
    }

    default: return false;
    }
}

bool nlohmann::detail::binary_reader::get_msgpack_array(const size_t len)
{
    if (!sax->start_array(len)) {
        return false;
    }

    for (size_t i = 0; i < len; ++i) {
        if (!parse_msgpack_internal()) {
            return false;
        }
    }

    return sax->end_array();
}

bool nlohmann::detail::binary_reader::get_msgpack_object(const size_t len)
{
    if (!sax->start_object(len)) {
        return false;
    }

    std::string key;
    for (size_t i = 0; i < len; ++i) {
        get();
        if (!get_msgpack_string(key) || !sax->key(key)) {
            return false;
        }

        if (!parse_msgpack_internal()) {
            return false;
        }
        key.clear();
    }

    return sax->end_object();
}

bool nlohmann::detail::binary_reader::parse_ubjson_internal(const bool get_char)
{
    return get_ubjson_value(get_char ? get_ignore_noop() : current);
}

bool nlohmann::detail::binary_reader::get_ubjson_string(std::string &result, const bool get_char)
{
    if (get_char) {
        get();
    }

    if (!unexpect_eof(input_format, "value")) {
        return false;
    }

    switch (current) {
    case 'U': {
        uint8_t len{};
        return get_number(input_format, len) && get_string(input_format, len, result);
    }

    case 'i': {
        std::int8_t len{};
        return get_number(input_format, len) && get_string(input_format, len, result);
    }

    case 'I': {
        std::int16_t len{};
        return get_number(input_format, len) && get_string(input_format, len, result);
    }

    case 'l': {
        int32_t len{};
        return get_number(input_format, len) && get_string(input_format, len, result);
    }

    case 'L': {
        int64_t len{};
        return get_number(input_format, len) && get_string(input_format, len, result);
    }

    case 'u': {
        if (input_format != input_format_t::bjdata) {
            break;
        }
        std::uint16_t len{};
        return get_number(input_format, len) && get_string(input_format, len, result);
    }

    case 'm': {
        if (input_format != input_format_t::bjdata) {
            break;
        }
        std::uint32_t len{};
        return get_number(input_format, len) && get_string(input_format, len, result);
    }

    case 'M': {
        if (input_format != input_format_t::bjdata) {
            break;
        }
        uint64_t len{};
        return get_number(input_format, len) && get_string(input_format, len, result);
    }

    default: break;
    }
    auto last_token = get_token_string();
    std::string message;

    if (input_format != input_format_t::bjdata) {
        message = "expected length type specification (U, i, I, l, L); last byte: 0x" + last_token;
    } else {
        message = "expected length type specification (U, i, u, I, m, l, M, L); last byte: 0x"
                  + last_token;
    }
    return sax->parse_error(chars_read, last_token,
                            parse_error::create(113, chars_read,
                                                exception_message(input_format, message,
                                                                  "string")));
}

bool nlohmann::detail::binary_reader::get_ubjson_ndarray_size(std::vector<size_t> &dim)
{
    std::pair<size_t, uint64_t> size_and_type;
    size_t dimlen = 0;
    bool no_ndarray = true;

    if (!get_ubjson_size_type(size_and_type, no_ndarray)) {
        return false;
    }

    if (size_and_type.first != npos) {
        if (size_and_type.second != 0) {
            if (size_and_type.second != 'N') {
                for (size_t i = 0; i < size_and_type.first; ++i) {
                    if (!get_ubjson_size_value(dimlen, no_ndarray, size_and_type.second)) {
                        return false;
                    }
                    dim.push_back(dimlen);
                }
            }
        } else {
            for (size_t i = 0; i < size_and_type.first; ++i) {
                if (!get_ubjson_size_value(dimlen, no_ndarray)) {
                    return false;
                }
                dim.push_back(dimlen);
            }
        }
    } else {
        while (current != ']') {
            if (!get_ubjson_size_value(dimlen, no_ndarray, current)) {
                return false;
            }
            dim.push_back(dimlen);
            get_ignore_noop();
        }
    }
    return true;
}

bool nlohmann::detail::binary_reader::get_ubjson_size_value(size_t &result, bool &is_ndarray,
                                                            uint64_t prefix)
{
    if (prefix == 0) {
        prefix = get_ignore_noop();
    }

    switch (prefix) {
    case 'U': {
        uint8_t number{};
        if (!get_number(input_format, number)) {
            return false;
        }
        result = static_cast<size_t>(number);
        return true;
    }

    case 'i': {
        std::int8_t number{};
        if (!get_number(input_format, number)) {
            return false;
        }
        if (number < 0) {
            return sax->parse_error(
                chars_read, get_token_string(),
                parse_error::create(
                    113, chars_read,
                    exception_message(input_format,
                                      "count in an optimized container must be positive", "size")));
        }
        result = static_cast<size_t>(number);
        return true;
    }

    case 'I': {
        std::int16_t number{};
        if (!get_number(input_format, number)) {
            return false;
        }
        if (number < 0) {
            return sax->parse_error(
                chars_read, get_token_string(),
                parse_error::create(
                    113, chars_read,
                    exception_message(input_format,
                                      "count in an optimized container must be positive", "size")));
        }
        result = static_cast<size_t>(number);
        return true;
    }

    case 'l': {
        int32_t number{};
        if (!get_number(input_format, number)) {
            return false;
        }
        if (number < 0) {
            return sax->parse_error(
                chars_read, get_token_string(),
                parse_error::create(
                    113, chars_read,
                    exception_message(input_format,
                                      "count in an optimized container must be positive", "size")));
        }
        result = static_cast<size_t>(number);
        return true;
    }

    case 'L': {
        int64_t number{};
        if (!get_number(input_format, number)) {
            return false;
        }
        if (number < 0) {
            return sax->parse_error(
                chars_read, get_token_string(),
                parse_error::create(
                    113, chars_read,
                    exception_message(input_format,
                                      "count in an optimized container must be positive", "size")));
        }
        if (number < 0) {
            return sax
                ->parse_error(chars_read, get_token_string(),
                              out_of_range::create(408, exception_message(input_format,
                                                                          "integer value overflow",
                                                                          "size")));
        }
        result = static_cast<size_t>(number);
        return true;
    }

    case 'u': {
        if (input_format != input_format_t::bjdata) {
            break;
        }
        std::uint16_t number{};
        if (!get_number(input_format, number)) {
            return false;
        }
        result = static_cast<size_t>(number);
        return true;
    }

    case 'm': {
        if (input_format != input_format_t::bjdata) {
            break;
        }
        std::uint32_t number{};
        if (!get_number(input_format, number)) {
            return false;
        }
        result = static_cast<size_t>(number);
        return true;
    }

    case 'M': {
        if (input_format != input_format_t::bjdata) {
            break;
        }
        uint64_t number{};
        if (!get_number(input_format, number)) {
            return false;
        }
        result = static_cast<size_t>(number);
        return true;
    }

    case '[': {
        if (input_format != input_format_t::bjdata) {
            break;
        }
        if (is_ndarray) {
            return sax->parse_error(
                chars_read, get_token_string(),
                parse_error::create(113, chars_read,
                                    exception_message(input_format,
                                                      "ndarray dimensional vector is not allowed",
                                                      "size")));
        }
        std::vector<size_t> dim;
        if (!get_ubjson_ndarray_size(dim)) {
            return false;
        }
        if (dim.size() == 1 || (dim.size() == 2 && dim.at(0) == 1)) {
            result = dim.at(dim.size() - 1);
            return true;
        }
        if (!dim.empty()) {
            for (auto i : dim) {
                if (i == 0) {
                    result = 0;
                    return true;
                }
            }

            std::string key = "_ArraySize_";
            if (!sax->start_object(3) || !sax->key(key) || !sax->start_array(dim.size())) {
                return false;
            }
            result = 1;
            for (auto i : dim) {
                result *= i;
                if (result == 0 || result == npos) {
                    return sax->parse_error(
                        chars_read, get_token_string(),
                        out_of_range::create(
                            408,
                            exception_message(input_format,
                                              "excessive ndarray size caused overflow", "size")));
                }
                if (!sax->number_integer(i)) {
                    return false;
                }
            }
            is_ndarray = true;
            return sax->end_array();
        }
        result = 0;
        return true;
    }

    default: break;
    }
    auto last_token = get_token_string();
    std::string message;

    if (input_format != input_format_t::bjdata) {
        message = "expected length type specification (U, i, I, l, L) after '#'; last byte: 0x"
                  + last_token;
    } else {
        message = "expected length type specification (U, i, u, I, m, l, M, L) after '#'; "
                  "last byte: 0x"
                  + last_token;
    }
    return sax->parse_error(chars_read, last_token,
                            parse_error::create(113, chars_read,
                                                exception_message(input_format, message, "size")));
}

bool nlohmann::detail::binary_reader::get_ubjson_size_type(std::pair<size_t, uint64_t> &result,
                                                           bool inside_ndarray)
{
    result.first = npos;
    result.second = 0;
    bool is_ndarray = false;

    get_ignore_noop();

    if (current == '$') {
        result.second = get();
        if (input_format == input_format_t::bjdata
            && std::binary_search(bjd_optimized_type_markers.begin(),
                                  bjd_optimized_type_markers.end(), result.second)) {
            auto last_token = get_token_string();
            return sax->parse_error(
                chars_read, last_token,
                parse_error::create(
                    112, chars_read,
                    exception_message(input_format,
                                      concat("marker 0x", last_token,
                                             " is not a permitted optimized array type"),
                                      "type")));
        }

        if (!unexpect_eof(input_format, "type")) {
            return false;
        }

        get_ignore_noop();
        if (current != '#') {
            if (!unexpect_eof(input_format, "value")) {
                return false;
            }
            auto last_token = get_token_string();
            return sax->parse_error(
                chars_read, last_token,
                parse_error::create(112, chars_read,
                                    exception_message(input_format,
                                                      concat("expected '#' after type "
                                                             "information; last byte: 0x",
                                                             last_token),
                                                      "size")));
        }

        const bool is_error = get_ubjson_size_value(result.first, is_ndarray);
        if (input_format == input_format_t::bjdata && is_ndarray) {
            if (inside_ndarray) {
                return sax->parse_error(
                    chars_read, get_token_string(),
                    parse_error::create(112, chars_read,
                                        exception_message(input_format,
                                                          "ndarray can not be recursive", "size")));
            }
            result.second |= (1 << 8);
        }
        return is_error;
    }

    if (current == '#') {
        const bool is_error = get_ubjson_size_value(result.first, is_ndarray);
        if (input_format == input_format_t::bjdata && is_ndarray) {
            return sax->parse_error(
                chars_read, get_token_string(),
                parse_error::create(112, chars_read,
                                    exception_message(input_format,
                                                      "ndarray requires both type and size",
                                                      "size")));
        }
        return is_error;
    }

    return true;
}

bool nlohmann::detail::binary_reader::get_ubjson_value(const uint64_t prefix)
{
    switch (prefix) {
    case uint64_t(EOF): return unexpect_eof(input_format, "value");

    case 'T': return sax->boolean(true);
    case 'F': return sax->boolean(false);

    case 'Z': return sax->null();

    case 'U': {
        uint8_t number{};
        return get_number(input_format, number) && sax->number_integer(number);
    }

    case 'i': {
        std::int8_t number{};
        return get_number(input_format, number) && sax->number_integer(number);
    }

    case 'I': {
        std::int16_t number{};
        return get_number(input_format, number) && sax->number_integer(number);
    }

    case 'l': {
        int32_t number{};
        return get_number(input_format, number) && sax->number_integer(number);
    }

    case 'L': {
        int64_t number{};
        return get_number(input_format, number) && sax->number_integer(number);
    }

    case 'u': {
        if (input_format != input_format_t::bjdata) {
            break;
        }
        std::uint16_t number{};
        return get_number(input_format, number) && sax->number_integer(number);
    }

    case 'm': {
        if (input_format != input_format_t::bjdata) {
            break;
        }
        std::uint32_t number{};
        return get_number(input_format, number) && sax->number_integer(number);
    }

    case 'M': {
        if (input_format != input_format_t::bjdata) {
            break;
        }
        int64_t number{};
        return get_number(input_format, number) && sax->number_integer(number);
    }

    case 'h': {
        if (input_format != input_format_t::bjdata) {
            break;
        }
        const auto byte1_raw = get();
        if (!unexpect_eof(input_format, "number")) {
            return false;
        }
        const auto byte2_raw = get();
        if (!unexpect_eof(input_format, "number")) {
            return false;
        }

        const auto byte1 = static_cast<unsigned char>(byte1_raw);
        const auto byte2 = static_cast<unsigned char>(byte2_raw);

        const auto half = static_cast<unsigned int>((byte2 << 8u) + byte1);
        const double val = [&half] {
            const int exp = (half >> 10u) & 0x1Fu;
            const unsigned int mant = half & 0x3FFu;

            switch (exp) {
            case 0: return std::ldexp(mant, -24);
            case 31:
                return (mant == 0) ? std::numeric_limits<double>::infinity()
                                   : std::numeric_limits<double>::quiet_NaN();
            default: return std::ldexp(mant + 1024, exp - 25);
            }
        }();
        return sax->number_float((half & 0x8000u) != 0 ? static_cast<double>(-val)
                                                       : static_cast<double>(val),
                                 "");
    }

    case 'd': {
        float number{};
        return get_number(input_format, number)
               && sax->number_float(static_cast<double>(number), "");
    }

    case 'D': {
        double number{};
        return get_number(input_format, number)
               && sax->number_float(static_cast<double>(number), "");
    }

    case 'H': {
        return get_ubjson_high_precision_number();
    }

    case 'C': {
        get();
        if (!unexpect_eof(input_format, "char")) {
            return false;
        }
        if (current > 127) {
            auto last_token = get_token_string();
            return sax->parse_error(
                chars_read, last_token,
                parse_error::create(113, chars_read,
                                    exception_message(input_format,
                                                      concat("byte after 'C' must be in range "
                                                             "0x00..0x7F; last byte: 0x",
                                                             last_token),
                                                      "char")));
        }
        std::string s(1, static_cast<typename std::string::value_type>(current));
        return sax->string(s);
    }

    case 'S': {
        std::string s;
        return get_ubjson_string(s) && sax->string(s);
    }

    case '[': return get_ubjson_array();

    case '{': return get_ubjson_object();

    default: break;
    }
    auto last_token = get_token_string();
    return sax->parse_error(chars_read, last_token,
                            parse_error::create(112, chars_read,
                                                exception_message(input_format,
                                                                  "invalid byte: 0x" + last_token,
                                                                  "value")));
}

bool nlohmann::detail::binary_reader::get_ubjson_array()
{
    std::pair<size_t, uint64_t> size_and_type;
    if (!get_ubjson_size_type(size_and_type)) {
        return false;
    }

    if (input_format == input_format_t::bjdata && size_and_type.first != npos
        && (size_and_type.second & (1 << 8)) != 0) {
        size_and_type.second &= ~(static_cast<uint64_t>(1) << 8);
        auto it = std::lower_bound(bjd_types_map.begin(), bjd_types_map.end(),
                                   size_and_type.second,
                                   [](const bjd_type &p, uint64_t t) { return p.first < t; });
        std::string key = "_ArrayType_";
        if (it == bjd_types_map.end() || it->first != size_and_type.second) {
            auto last_token = get_token_string();
            return sax->parse_error(chars_read, last_token,
                                    parse_error::create(112, chars_read,
                                                        exception_message(input_format,
                                                                          "invalid byte: 0x"
                                                                              + last_token,
                                                                          "type")));
        }

        std::string type = it->second;
        if (!sax->key(key) || !sax->string(type)) {
            return false;
        }

        if (size_and_type.second == 'C') {
            size_and_type.second = 'U';
        }

        key = "_ArrayData_";
        if (!sax->key(key) || !sax->start_array(size_and_type.first)) {
            return false;
        }

        for (size_t i = 0; i < size_and_type.first; ++i) {
            if (!get_ubjson_value(size_and_type.second)) {
                return false;
            }
        }

        return (sax->end_array() && sax->end_object());
    }

    if (size_and_type.first != npos) {
        if (!sax->start_array(size_and_type.first)) {
            return false;
        }

        if (size_and_type.second != 0) {
            if (size_and_type.second != 'N') {
                for (size_t i = 0; i < size_and_type.first; ++i) {
                    if (!get_ubjson_value(size_and_type.second)) {
                        return false;
                    }
                }
            }
        } else {
            for (size_t i = 0; i < size_and_type.first; ++i) {
                if (!parse_ubjson_internal()) {
                    return false;
                }
            }
        }
    } else {
        if (!sax->start_array(static_cast<size_t>(-1))) {
            return false;
        }

        while (current != ']') {
            if (!parse_ubjson_internal(false)) {
                return false;
            }
            get_ignore_noop();
        }
    }

    return sax->end_array();
}

bool nlohmann::detail::binary_reader::get_ubjson_object()
{
    std::pair<size_t, uint64_t> size_and_type;
    if (!get_ubjson_size_type(size_and_type)) {
        return false;
    }

    if (input_format == input_format_t::bjdata && size_and_type.first != npos
        && (size_and_type.second & (1 << 8)) != 0) {
        auto last_token = get_token_string();
        return sax->parse_error(
            chars_read, last_token,
            parse_error::create(112, chars_read,
                                exception_message(input_format,
                                                  "BJData object does not support ND-array "
                                                  "size in optimized format",
                                                  "object")));
    }

    std::string key;
    if (size_and_type.first != npos) {
        if (!sax->start_object(size_and_type.first)) {
            return false;
        }

        if (size_and_type.second != 0) {
            for (size_t i = 0; i < size_and_type.first; ++i) {
                if (!get_ubjson_string(key) || !sax->key(key)) {
                    return false;
                }
                if (!get_ubjson_value(size_and_type.second)) {
                    return false;
                }
                key.clear();
            }
        } else {
            for (size_t i = 0; i < size_and_type.first; ++i) {
                if (!get_ubjson_string(key) || !sax->key(key)) {
                    return false;
                }
                if (!parse_ubjson_internal()) {
                    return false;
                }
                key.clear();
            }
        }
    } else {
        if (!sax->start_object(static_cast<size_t>(-1))) {
            return false;
        }

        while (current != '}') {
            if (!get_ubjson_string(key, false) || !sax->key(key)) {
                return false;
            }
            if (!parse_ubjson_internal()) {
                return false;
            }
            get_ignore_noop();
            key.clear();
        }
    }

    return sax->end_object();
}

bool nlohmann::detail::binary_reader::get_ubjson_high_precision_number()
{
    size_t size{};
    bool no_ndarray = true;
    auto res = get_ubjson_size_value(size, no_ndarray);
    if (!res) {
        return res;
    }

    std::string number_vector;
    for (size_t i = 0; i < size; ++i) {
        get();
        if (!unexpect_eof(input_format, "number")) {
            return false;
        }
        number_vector.push_back(static_cast<char>(current));
    }

    auto number_lexer = detail::lexer(detail::input_adapter(number_vector), false);
    const auto result_number = number_lexer.scan();
    const auto number_string = number_lexer.get_token_string();
    const auto result_remainder = number_lexer.scan();

    if (result_remainder != lexer_token_type::end_of_input) {
        return sax->parse_error(
            chars_read, number_string,
            parse_error::create(115, chars_read,
                                exception_message(input_format,
                                                  concat("invalid number text: ",
                                                         number_lexer.get_token_string()),
                                                  "high-precision number")));
    }

    switch (result_number) {
    case lexer_token_type::value_integer: return sax->number_integer(number_lexer.get_number_integer());
    case lexer_token_type::value_float:
        return sax->number_float(number_lexer.get_number_float(), number_string);
    case lexer_token_type::uninitialized:
    case lexer_token_type::literal_true:
    case lexer_token_type::literal_false:
    case lexer_token_type::literal_null:
    case lexer_token_type::value_string:
    case lexer_token_type::begin_array:
    case lexer_token_type::begin_object:
    case lexer_token_type::end_array:
    case lexer_token_type::end_object:
    case lexer_token_type::name_separator:
    case lexer_token_type::value_separator:
    case lexer_token_type::parse_error:
    case lexer_token_type::end_of_input:
    case lexer_token_type::literal_or_value:
    default:
        return sax->parse_error(
            chars_read, number_string,
            parse_error::create(115, chars_read,
                                exception_message(input_format,
                                                  concat("invalid number text: ",
                                                         number_lexer.get_token_string()),
                                                  "high-precision number")));
    }
}

uint64_t nlohmann::detail::binary_reader::get()
{
    ++chars_read;
    return current = ia->get_character();
}

uint64_t nlohmann::detail::binary_reader::get_ignore_noop()
{
    do {
        get();
    } while (current == 'N');

    return current;
}

bool nlohmann::detail::binary_reader::get_string(const input_format_t format, const int32_t len,
                                                 std::string &result)
{
    bool success = true;
    for (int32_t i = 0; i < len; i++) {
        get();
        if (!unexpect_eof(format, "string")) {
            success = false;
            break;
        }
        result.push_back(static_cast<typename std::string::value_type>(current));
    }
    return success;
}

bool nlohmann::detail::binary_reader::get_binary(const input_format_t format, const int32_t len,
                                                 byte_container_with_subtype &result)
{
    bool success = true;
    for (int32_t i = 0; i < len; i++) {
        get();
        if (!unexpect_eof(format, "binary")) {
            success = false;
            break;
        }
        result.push_back(static_cast<uint8_t>(current));
    }
    return success;
}

bool nlohmann::detail::binary_reader::unexpect_eof(const input_format_t format,
                                                   const char *context) const
{
    if (current == uint64_t(EOF)) {
        return sax->parse_error(chars_read, "<end of file>",
                                parse_error::create(110, chars_read,
                                                    exception_message(format,
                                                                      "unexpected end of input",
                                                                      context)));
    }
    return true;
}

std::string nlohmann::detail::binary_reader::get_token_string() const
{
    std::array<char, 3> cr{{}};
    static_cast<void>(
        (std::snprintf)(cr.data(), cr.size(), "%.2hhX", static_cast<unsigned char>(current)));
    return std::string{cr.data()};
}

std::string nlohmann::detail::binary_reader::exception_message(const input_format_t format,
                                                               const std::string &detail,
                                                               const std::string &context) const
{
    std::string error_msg = "syntax error while parsing ";

    switch (format) {
    case input_format_t::cbor: error_msg += "CBOR"; break;

    case input_format_t::msgpack: error_msg += "MessagePack"; break;

    case input_format_t::ubjson: error_msg += "UBJSON"; break;

    case input_format_t::bson: error_msg += "BSON"; break;

    case input_format_t::bjdata: error_msg += "BJData"; break;

    case input_format_t::json:
    default: return {};
    }

    return concat(error_msg, ' ', context, ": ", detail);
}
