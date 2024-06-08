//     __ _____ _____ _____
//  __|  |   __|     |   | |  JSON for Modern C++
// |  |  |__   |  |  | | | |  version 3.11.3
// |_____|_____|_____|_|___|  https://github.com/nlohmann/json
//
// SPDX-FileCopyrightText: 2013-2023 Niels Lohmann <https://nlohmann.me>
// SPDX-License-Identifier: MIT

#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <memory>

#include "nlohmann/detail/input/input_format_t.h"
#include "nlohmann/ordered_map.hpp"

namespace nlohmann {
class ordered_json;
class byte_container_with_subtype;

namespace detail {
class output_adapter_protocol;

class binary_writer
{
public:
    explicit binary_writer(std::shared_ptr<output_adapter_protocol> adapter);
    ~binary_writer();

    void write_bson(const ordered_json &j);
    void write_cbor(const ordered_json &j);
    void write_msgpack(const ordered_json &j);
    void write_ubjson(const ordered_json &j, const bool use_count, const bool use_type,
                      const bool add_prefix = true, const bool use_bjdata = false);

private:
    static size_t calc_bson_entry_header_size(const std::string &name, const ordered_json &j);
    void write_bson_entry_header(const std::string &name, const uint8_t element_type);
    void write_bson_boolean(const std::string &name, const bool value);
    void write_bson_double(const std::string &name, const double value);
    static size_t calc_bson_string_size(const std::string &value);
    void write_bson_string(const std::string &name, const std::string &value);
    void write_bson_null(const std::string &name) { write_bson_entry_header(name, 0x0A); }
    static size_t calc_bson_integer_size(const int64_t value);
    void write_bson_integer(const std::string &name, const int64_t value);
    static size_t calc_bson_unsigned_size(const uint64_t value) noexcept;

    void write_bson_unsigned(const std::string &name, const ordered_json &j);
    void write_bson_object_entry(const std::string &name,
                                 const nlohmann::ordered_map<std::string, ordered_json> &value);
    static size_t calc_bson_array_size(const std::vector<ordered_json> &value);
    static size_t calc_bson_binary_size(const nlohmann::byte_container_with_subtype &value);
    void write_bson_array(const std::string &name, const std::vector<ordered_json> &value);
    void write_bson_binary(const std::string &name, const byte_container_with_subtype &value);
    static size_t calc_bson_element_size(const std::string &name, const ordered_json &j);
    void write_bson_element(const std::string &name, const ordered_json &j);
    static size_t calc_bson_object_size(
        const nlohmann::ordered_map<std::string, ordered_json> &value);
    void write_bson_object(const nlohmann::ordered_map<std::string, ordered_json> &value);
    static constexpr char get_cbor_float_prefix(float) { return to_char_type(0xFA); }
    static constexpr char get_cbor_float_prefix(double) { return to_char_type(0xFB); }
    static constexpr char get_msgpack_float_prefix(float) { return to_char_type(0xCA); }
    static constexpr char get_msgpack_float_prefix(double) { return to_char_type(0xCB); }

    void write_number_with_ubjson_prefix(const double n, const bool add_prefix,
                                         const bool use_bjdata);
    void write_number_with_ubjson_prefix(const size_t n, const bool add_prefix,
                                         const bool use_bjdata);
    void write_number_with_ubjson_prefix(const int64_t n, const bool add_prefix,
                                         const bool use_bjdata);

    char ubjson_prefix(const ordered_json &j, const bool use_bjdata) const noexcept;
    static constexpr char get_ubjson_float_prefix(float) { return 'd'; }
    static constexpr char get_ubjson_float_prefix(double) { return 'D'; }

    bool write_bjdata_ndarray(const nlohmann::ordered_map<std::string, ordered_json> &value,
                              const bool use_count, const bool use_type);

    template<typename NumberType>
    void write_number(const NumberType n, const bool OutputIsLittleEndian = false)
    {
        std::array<char, sizeof(NumberType)> vec{};
        std::memcpy(vec.data(), &n, sizeof(NumberType));
        if (is_little_endian != OutputIsLittleEndian) {
            std::reverse(vec.begin(), vec.end());
        }
        write_characters(vec.data(), sizeof(NumberType));
    }
    void write_characters(const char *s, size_t length);

    void write_compact_float(const double n, detail::input_format_t format);

public:
    static constexpr char to_char_type(uint8_t x) noexcept { return static_cast<char>(x); }

private:
    const bool is_little_endian = little_endianness();

    std::shared_ptr<output_adapter_protocol> oa;
};

} // namespace detail
} // namespace nlohmann
