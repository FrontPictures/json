//     __ _____ _____ _____
//  __|  |   __|     |   | |  JSON for Modern C++
// |  |  |__   |  |  | | | |  version 3.11.3
// |_____|_____|_____|_|___|  https://github.com/nlohmann/json
//
// SPDX-FileCopyrightText: 2013-2023 Niels Lohmann <https://nlohmann.me>
// SPDX-License-Identifier: MIT

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "input_format_t.h"

namespace nlohmann {
class byte_container_with_subtype;
class json_sax;

namespace detail {
class iterator_input_adapter;

class binary_reader
{
public:
    explicit binary_reader(const iterator_input_adapter &adapter,
                           const input_format_t format = input_format_t::json) noexcept;
    ~binary_reader();

    bool sax_parse(const input_format_t format, json_sax *sax_, const bool strict = true,
                   const cbor_tag_handler_t tag_handler = cbor_tag_handler_t::error);

private:
    bool parse_bson_internal();
    bool get_bson_cstr(std::string &result);
    bool get_bson_string(const int32_t len, std::string &result);
    bool get_bson_binary(const int32_t len, byte_container_with_subtype &result);
    bool parse_bson_element_internal(const uint64_t element_type,
                                     const size_t element_type_parse_position);
    bool parse_bson_element_list(const bool is_array);
    bool parse_bson_array();
    bool parse_cbor_internal(const bool get_char, const cbor_tag_handler_t tag_handler);
    bool get_cbor_string(std::string &result);
    bool get_cbor_binary(byte_container_with_subtype &result);
    bool get_cbor_array(const size_t len, const cbor_tag_handler_t tag_handler);
    bool get_cbor_object(const size_t len, const cbor_tag_handler_t tag_handler);
    bool parse_msgpack_internal();
    bool get_msgpack_string(std::string &result);
    bool get_msgpack_binary(byte_container_with_subtype &result);
    bool get_msgpack_array(const size_t len);
    bool get_msgpack_object(const size_t len);
    bool parse_ubjson_internal(const bool get_char = true);
    bool get_ubjson_string(std::string &result, const bool get_char = true);
    bool get_ubjson_ndarray_size(std::vector<size_t> &dim);
    bool get_ubjson_size_value(size_t &result, bool &is_ndarray, uint64_t prefix = 0);
    bool get_ubjson_size_type(std::pair<size_t, uint64_t> &result, bool inside_ndarray = false);
    bool get_ubjson_value(const uint64_t prefix);
    bool get_ubjson_array();
    bool get_ubjson_object();
    bool get_ubjson_high_precision_number();
    uint64_t get();
    uint64_t get_ignore_noop();

    template<typename NumberType, bool InputIsLittleEndian = false>
    bool get_number(const input_format_t format, NumberType &result)
    {
        std::array<uint8_t, sizeof(NumberType)> vec{};
        for (size_t i = 0; i < sizeof(NumberType); ++i) {
            get();
            if (!unexpect_eof(format, "number")) {
                return false;
            }

            if (is_little_endian != (InputIsLittleEndian || format == input_format_t::bjdata)) {
                vec[sizeof(NumberType) - i - 1] = static_cast<uint8_t>(current);
            } else {
                vec[i] = static_cast<uint8_t>(current);
            }
        }

        std::memcpy(&result, vec.data(), sizeof(NumberType));
        return true;
    }

    bool get_string(const input_format_t format, const int32_t len, std::string &result);
    bool get_binary(const input_format_t format, const int32_t len,
                    byte_container_with_subtype &result);
    bool unexpect_eof(const input_format_t format, const char *context) const;
    std::string get_token_string() const;
    std::string exception_message(const input_format_t format, const std::string &detail,
                                  const std::string &context) const;

#ifndef JSON_TESTS_PRIVATE
private:
#else
public:
#endif
    static constexpr size_t npos = static_cast<size_t>(-1);
    std::unique_ptr<iterator_input_adapter> ia;
    uint64_t current = uint64_t(EOF);
    size_t chars_read = 0;
    const bool is_little_endian = little_endianness();
    const input_format_t input_format = input_format_t::json;
    json_sax *sax = nullptr;
    const std::array<uint64_t, 8> bjd_optimized_type_markers = {'F', 'H', 'N', 'S',
                                                                'T', 'Z', '[', '{'};
    using bjd_type = std::pair<uint64_t, std::string>;
    const std::array<bjd_type, 11> bjd_types_map
        = {bjd_type{'C', "char"},   bjd_type{'D', "double"}, bjd_type{'I', "int16"},
           bjd_type{'L', "int64"},  bjd_type{'M', "uint64"}, bjd_type{'U', "uint8"},
           bjd_type{'d', "single"}, bjd_type{'i', "int8"},   bjd_type{'l', "int32"},
           bjd_type{'m', "uint32"}, bjd_type{'u', "uint16"}};
};

} // namespace detail
} // namespace nlohmann
