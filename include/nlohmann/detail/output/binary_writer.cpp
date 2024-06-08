#include "binary_writer.hpp"
#include <map>
#include <numeric>
#include <cmath>
#include "nlohmann/detail/string_concat.hpp"
#include "nlohmann/json.hpp"
#include "nlohmann/detail/output/output_adapters.hpp"

nlohmann::detail::binary_writer::binary_writer(std::shared_ptr<output_adapter_protocol> adapter) : oa(std::move(adapter))
{}

nlohmann::detail::binary_writer::~binary_writer() = default;

void nlohmann::detail::binary_writer::write_bson(const ordered_json &j)
{
    switch (j.type()) {
    case value_t::object: {
        write_bson_object(*j.m_data.m_value.object);
        break;
    }

    case value_t::null:
    case value_t::array:
    case value_t::string:
    case value_t::boolean:
    case value_t::number_integer:
    case value_t::number_float:
    case value_t::binary:
    case value_t::discarded:
    default: {
        throw(type_error::create(
            317, concat("to serialize to BSON, top-level type must be object, but is ",
                        j.type_name())));
    }
    }
}

void nlohmann::detail::binary_writer::write_cbor(const ordered_json &j)
{
    switch (j.type()) {
    case value_t::null: {
        oa->write_character(to_char_type(0xF6));
        break;
    }

    case value_t::boolean: {
        oa->write_character(j.m_data.m_value.boolean ? to_char_type(0xF5) : to_char_type(0xF4));
        break;
    }

    case value_t::number_integer: {
        if (j.m_data.m_value.number_integer >= 0) {
            if (j.m_data.m_value.number_integer <= 0x17) {
                write_number(static_cast<uint8_t>(j.m_data.m_value.number_integer));
            } else if (j.m_data.m_value.number_integer <= (std::numeric_limits<uint8_t>::max)()) {
                oa->write_character(to_char_type(0x18));
                write_number(static_cast<uint8_t>(j.m_data.m_value.number_integer));
            } else if (j.m_data.m_value.number_integer
                       <= (std::numeric_limits<std::uint16_t>::max)()) {
                oa->write_character(to_char_type(0x19));
                write_number(static_cast<std::uint16_t>(j.m_data.m_value.number_integer));
            } else if (j.m_data.m_value.number_integer
                       <= (std::numeric_limits<std::uint32_t>::max)()) {
                oa->write_character(to_char_type(0x1A));
                write_number(static_cast<std::uint32_t>(j.m_data.m_value.number_integer));
            } else {
                oa->write_character(to_char_type(0x1B));
                write_number(static_cast<uint64_t>(j.m_data.m_value.number_integer));
            }
        } else {
            const auto positive_number = -1 - j.m_data.m_value.number_integer;
            if (j.m_data.m_value.number_integer >= -24) {
                write_number(static_cast<uint8_t>(0x20 + positive_number));
            } else if (positive_number <= (std::numeric_limits<uint8_t>::max)()) {
                oa->write_character(to_char_type(0x38));
                write_number(static_cast<uint8_t>(positive_number));
            } else if (positive_number <= (std::numeric_limits<std::uint16_t>::max)()) {
                oa->write_character(to_char_type(0x39));
                write_number(static_cast<std::uint16_t>(positive_number));
            } else if (positive_number <= (std::numeric_limits<std::uint32_t>::max)()) {
                oa->write_character(to_char_type(0x3A));
                write_number(static_cast<std::uint32_t>(positive_number));
            } else {
                oa->write_character(to_char_type(0x3B));
                write_number(static_cast<uint64_t>(positive_number));
            }
        }
        break;
    }

    case value_t::number_float: {
        if (std::isnan(j.m_data.m_value.number_float)) {
            oa->write_character(to_char_type(0xF9));
            oa->write_character(to_char_type(0x7E));
            oa->write_character(to_char_type(0x00));
        } else if (std::isinf(j.m_data.m_value.number_float)) {
            oa->write_character(to_char_type(0xf9));
            oa->write_character(j.m_data.m_value.number_float > 0 ? to_char_type(0x7C)
                                                                  : to_char_type(0xFC));
            oa->write_character(to_char_type(0x00));
        } else {
            write_compact_float(j.m_data.m_value.number_float, detail::input_format_t::cbor);
        }
        break;
    }

    case value_t::string: {
        const auto N = j.m_data.m_value.string->size();
        if (N <= 0x17) {
            write_number(static_cast<uint8_t>(0x60 + N));
        } else if (N <= (std::numeric_limits<uint8_t>::max)()) {
            oa->write_character(to_char_type(0x78));
            write_number(static_cast<uint8_t>(N));
        } else if (N <= (std::numeric_limits<std::uint16_t>::max)()) {
            oa->write_character(to_char_type(0x79));
            write_number(static_cast<std::uint16_t>(N));
        } else if (N <= (std::numeric_limits<std::uint32_t>::max)()) {
            oa->write_character(to_char_type(0x7A));
            write_number(static_cast<std::uint32_t>(N));
        }

        else if (N <= (std::numeric_limits<uint64_t>::max)()) {
            oa->write_character(to_char_type(0x7B));
            write_number(static_cast<uint64_t>(N));
        }

        oa->write_characters(reinterpret_cast<const char *>(j.m_data.m_value.string->c_str()),
                             j.m_data.m_value.string->size());
        break;
    }

    case value_t::array: {
        const auto N = j.m_data.m_value.array->size();
        if (N <= 0x17) {
            write_number(static_cast<uint8_t>(0x80 + N));
        } else if (N <= (std::numeric_limits<uint8_t>::max)()) {
            oa->write_character(to_char_type(0x98));
            write_number(static_cast<uint8_t>(N));
        } else if (N <= (std::numeric_limits<std::uint16_t>::max)()) {
            oa->write_character(to_char_type(0x99));
            write_number(static_cast<std::uint16_t>(N));
        } else if (N <= (std::numeric_limits<std::uint32_t>::max)()) {
            oa->write_character(to_char_type(0x9A));
            write_number(static_cast<std::uint32_t>(N));
        }

        else if (N <= (std::numeric_limits<uint64_t>::max)()) {
            oa->write_character(to_char_type(0x9B));
            write_number(static_cast<uint64_t>(N));
        }

        for (const auto &el : *j.m_data.m_value.array) {
            write_cbor(el);
        }
        break;
    }

    case value_t::binary: {
        if (j.m_data.m_value.binary->has_subtype()) {
            if (j.m_data.m_value.binary->subtype() <= (std::numeric_limits<uint8_t>::max)()) {
                write_number(static_cast<uint8_t>(0xd8));
                write_number(static_cast<uint8_t>(j.m_data.m_value.binary->subtype()));
            } else if (j.m_data.m_value.binary->subtype()
                       <= (std::numeric_limits<std::uint16_t>::max)()) {
                write_number(static_cast<uint8_t>(0xd9));
                write_number(static_cast<std::uint16_t>(j.m_data.m_value.binary->subtype()));
            } else if (j.m_data.m_value.binary->subtype()
                       <= (std::numeric_limits<std::uint32_t>::max)()) {
                write_number(static_cast<uint8_t>(0xda));
                write_number(static_cast<std::uint32_t>(j.m_data.m_value.binary->subtype()));
            } else if (j.m_data.m_value.binary->subtype()
                       <= (std::numeric_limits<uint64_t>::max)()) {
                write_number(static_cast<uint8_t>(0xdb));
                write_number(static_cast<uint64_t>(j.m_data.m_value.binary->subtype()));
            }
        }

        const auto N = j.m_data.m_value.binary->size();
        if (N <= 0x17) {
            write_number(static_cast<uint8_t>(0x40 + N));
        } else if (N <= (std::numeric_limits<uint8_t>::max)()) {
            oa->write_character(to_char_type(0x58));
            write_number(static_cast<uint8_t>(N));
        } else if (N <= (std::numeric_limits<std::uint16_t>::max)()) {
            oa->write_character(to_char_type(0x59));
            write_number(static_cast<std::uint16_t>(N));
        } else if (N <= (std::numeric_limits<std::uint32_t>::max)()) {
            oa->write_character(to_char_type(0x5A));
            write_number(static_cast<std::uint32_t>(N));
        }

        else if (N <= (std::numeric_limits<uint64_t>::max)()) {
            oa->write_character(to_char_type(0x5B));
            write_number(static_cast<uint64_t>(N));
        }

        oa->write_characters(reinterpret_cast<const char *>(j.m_data.m_value.binary->data()), N);

        break;
    }

    case value_t::object: {
        const auto N = j.m_data.m_value.object->size();
        if (N <= 0x17) {
            write_number(static_cast<uint8_t>(0xA0 + N));
        } else if (N <= (std::numeric_limits<uint8_t>::max)()) {
            oa->write_character(to_char_type(0xB8));
            write_number(static_cast<uint8_t>(N));
        } else if (N <= (std::numeric_limits<std::uint16_t>::max)()) {
            oa->write_character(to_char_type(0xB9));
            write_number(static_cast<std::uint16_t>(N));
        } else if (N <= (std::numeric_limits<std::uint32_t>::max)()) {
            oa->write_character(to_char_type(0xBA));
            write_number(static_cast<std::uint32_t>(N));
        }

        else if (N <= (std::numeric_limits<uint64_t>::max)()) {
            oa->write_character(to_char_type(0xBB));
            write_number(static_cast<uint64_t>(N));
        }

        for (const auto &el : *j.m_data.m_value.object) {
            write_cbor(el.first);
            write_cbor(el.second);
        }
        break;
    }

    case value_t::discarded:
    default: break;
    }
}

void nlohmann::detail::binary_writer::write_msgpack(const ordered_json &j)
{
    switch (j.type()) {
    case value_t::null: {
        oa->write_character(to_char_type(0xC0));
        break;
    }

    case value_t::boolean: {
        oa->write_character(j.m_data.m_value.boolean ? to_char_type(0xC3) : to_char_type(0xC2));
        break;
    }

    case value_t::number_integer: {
        if (j.m_data.m_value.number_integer >= 0) {
            if (j.m_data.m_value.number_integer < 128) {
                write_number(static_cast<uint8_t>(j.m_data.m_value.number_integer));
            } else if (j.m_data.m_value.number_integer <= (std::numeric_limits<uint8_t>::max)()) {
                oa->write_character(to_char_type(0xCC));
                write_number(static_cast<uint8_t>(j.m_data.m_value.number_integer));
            } else if (j.m_data.m_value.number_integer
                       <= (std::numeric_limits<std::uint16_t>::max)()) {
                oa->write_character(to_char_type(0xCD));
                write_number(static_cast<std::uint16_t>(j.m_data.m_value.number_integer));
            } else if (j.m_data.m_value.number_integer
                       <= (std::numeric_limits<std::uint32_t>::max)()) {
                oa->write_character(to_char_type(0xCE));
                write_number(static_cast<std::uint32_t>(j.m_data.m_value.number_integer));
            } else if (j.m_data.m_value.number_integer <= (std::numeric_limits<uint64_t>::max)()) {
                oa->write_character(to_char_type(0xCF));
                write_number(static_cast<uint64_t>(j.m_data.m_value.number_integer));
            }
        } else {
            if (j.m_data.m_value.number_integer >= -32) {
                write_number(static_cast<std::int8_t>(j.m_data.m_value.number_integer));
            } else if (j.m_data.m_value.number_integer >= (std::numeric_limits<std::int8_t>::min)()
                       && j.m_data.m_value.number_integer
                              <= (std::numeric_limits<std::int8_t>::max)()) {
                oa->write_character(to_char_type(0xD0));
                write_number(static_cast<std::int8_t>(j.m_data.m_value.number_integer));
            } else if (j.m_data.m_value.number_integer >= (std::numeric_limits<std::int16_t>::min)()
                       && j.m_data.m_value.number_integer
                              <= (std::numeric_limits<std::int16_t>::max)()) {
                oa->write_character(to_char_type(0xD1));
                write_number(static_cast<std::int16_t>(j.m_data.m_value.number_integer));
            } else if (j.m_data.m_value.number_integer >= (std::numeric_limits<int32_t>::min)()
                       && j.m_data.m_value.number_integer
                              <= (std::numeric_limits<int32_t>::max)()) {
                oa->write_character(to_char_type(0xD2));
                write_number(static_cast<int32_t>(j.m_data.m_value.number_integer));
            } else if (j.m_data.m_value.number_integer >= (std::numeric_limits<int64_t>::min)()
                       && j.m_data.m_value.number_integer
                              <= (std::numeric_limits<int64_t>::max)()) {
                oa->write_character(to_char_type(0xD3));
                write_number(static_cast<int64_t>(j.m_data.m_value.number_integer));
            }
        }
        break;
    }

    case value_t::number_float: {
        write_compact_float(j.m_data.m_value.number_float, detail::input_format_t::msgpack);
        break;
    }

    case value_t::string: {
        const auto N = j.m_data.m_value.string->size();
        if (N <= 31) {
            write_number(static_cast<uint8_t>(0xA0 | N));
        } else if (N <= (std::numeric_limits<uint8_t>::max)()) {
            oa->write_character(to_char_type(0xD9));
            write_number(static_cast<uint8_t>(N));
        } else if (N <= (std::numeric_limits<std::uint16_t>::max)()) {
            oa->write_character(to_char_type(0xDA));
            write_number(static_cast<std::uint16_t>(N));
        } else if (N <= (std::numeric_limits<std::uint32_t>::max)()) {
            oa->write_character(to_char_type(0xDB));
            write_number(static_cast<std::uint32_t>(N));
        }

        oa->write_characters(reinterpret_cast<const char *>(j.m_data.m_value.string->c_str()),
                             j.m_data.m_value.string->size());
        break;
    }

    case value_t::array: {
        const auto N = j.m_data.m_value.array->size();
        if (N <= 15) {
            write_number(static_cast<uint8_t>(0x90 | N));
        } else if (N <= (std::numeric_limits<std::uint16_t>::max)()) {
            oa->write_character(to_char_type(0xDC));
            write_number(static_cast<std::uint16_t>(N));
        } else if (N <= (std::numeric_limits<std::uint32_t>::max)()) {
            oa->write_character(to_char_type(0xDD));
            write_number(static_cast<std::uint32_t>(N));
        }

        for (const auto &el : *j.m_data.m_value.array) {
            write_msgpack(el);
        }
        break;
    }

    case value_t::binary: {
        const bool use_ext = j.m_data.m_value.binary->has_subtype();

        const auto N = j.m_data.m_value.binary->size();
        if (N <= (std::numeric_limits<uint8_t>::max)()) {
            uint8_t output_type{};
            bool fixed = true;
            if (use_ext) {
                switch (N) {
                case 1: output_type = 0xD4; break;
                case 2: output_type = 0xD5; break;
                case 4: output_type = 0xD6; break;
                case 8: output_type = 0xD7; break;
                case 16: output_type = 0xD8; break;
                default:
                    output_type = 0xC7;
                    fixed = false;
                    break;
                }

            } else {
                output_type = 0xC4;
                fixed = false;
            }

            oa->write_character(to_char_type(output_type));
            if (!fixed) {
                write_number(static_cast<uint8_t>(N));
            }
        } else if (N <= (std::numeric_limits<std::uint16_t>::max)()) {
            const uint8_t output_type = use_ext ? 0xC8 : 0xC5;

            oa->write_character(to_char_type(output_type));
            write_number(static_cast<std::uint16_t>(N));
        } else if (N <= (std::numeric_limits<std::uint32_t>::max)()) {
            const uint8_t output_type = use_ext ? 0xC9 : 0xC6;

            oa->write_character(to_char_type(output_type));
            write_number(static_cast<std::uint32_t>(N));
        }

        if (use_ext) {
            write_number(static_cast<std::int8_t>(j.m_data.m_value.binary->subtype()));
        }

        oa->write_characters(reinterpret_cast<const char *>(j.m_data.m_value.binary->data()), N);

        break;
    }

    case value_t::object: {
        const auto N = j.m_data.m_value.object->size();
        if (N <= 15) {
            write_number(static_cast<uint8_t>(0x80 | (N & 0xF)));
        } else if (N <= (std::numeric_limits<std::uint16_t>::max)()) {
            oa->write_character(to_char_type(0xDE));
            write_number(static_cast<std::uint16_t>(N));
        } else if (N <= (std::numeric_limits<std::uint32_t>::max)()) {
            oa->write_character(to_char_type(0xDF));
            write_number(static_cast<std::uint32_t>(N));
        }

        for (const auto &el : *j.m_data.m_value.object) {
            write_msgpack(el.first);
            write_msgpack(el.second);
        }
        break;
    }

    case value_t::discarded:
    default: break;
    }
}

void nlohmann::detail::binary_writer::write_ubjson(const ordered_json &j, const bool use_count,
                                                   const bool use_type, const bool add_prefix,
                                                   const bool use_bjdata)
{
    switch (j.type()) {
    case value_t::null: {
        if (add_prefix) {
            oa->write_character(to_char_type('Z'));
        }
        break;
    }

    case value_t::boolean: {
        if (add_prefix) {
            oa->write_character(j.m_data.m_value.boolean ? to_char_type('T') : to_char_type('F'));
        }
        break;
    }

    case value_t::number_integer: {
        write_number_with_ubjson_prefix(j.m_data.m_value.number_integer, add_prefix, use_bjdata);
        break;
    }

    case value_t::number_float: {
        write_number_with_ubjson_prefix(j.m_data.m_value.number_float, add_prefix, use_bjdata);
        break;
    }

    case value_t::string: {
        if (add_prefix) {
            oa->write_character(to_char_type('S'));
        }
        write_number_with_ubjson_prefix(j.m_data.m_value.string->size(), true, use_bjdata);
        oa->write_characters(reinterpret_cast<const char *>(j.m_data.m_value.string->c_str()),
                             j.m_data.m_value.string->size());
        break;
    }

    case value_t::array: {
        if (add_prefix) {
            oa->write_character(to_char_type('['));
        }

        bool prefix_required = true;
        if (use_type && !j.m_data.m_value.array->empty()) {
            const char first_prefix = ubjson_prefix(j.front(), use_bjdata);
            const bool same_prefix = std::all_of(j.begin() + 1, j.end(),
                                                 [this, first_prefix,
                                                  use_bjdata](const ordered_json &v) {
                                                     return ubjson_prefix(v, use_bjdata)
                                                            == first_prefix;
                                                 });

            std::vector<char> bjdx = {'[', '{', 'S', 'H', 'T', 'F', 'N', 'Z'};

            if (same_prefix
                && !(use_bjdata
                     && std::find(bjdx.begin(), bjdx.end(), first_prefix) != bjdx.end())) {
                prefix_required = false;
                oa->write_character(to_char_type('$'));
                oa->write_character(first_prefix);
            }
        }

        if (use_count) {
            oa->write_character(to_char_type('#'));
            write_number_with_ubjson_prefix(j.m_data.m_value.array->size(), true, use_bjdata);
        }

        for (const auto &el : *j.m_data.m_value.array) {
            write_ubjson(el, use_count, use_type, prefix_required, use_bjdata);
        }

        if (!use_count) {
            oa->write_character(to_char_type(']'));
        }

        break;
    }

    case value_t::binary: {
        if (add_prefix) {
            oa->write_character(to_char_type('['));
        }

        if (use_type && !j.m_data.m_value.binary->empty()) {
            oa->write_character(to_char_type('$'));
            oa->write_character('U');
        }

        if (use_count) {
            oa->write_character(to_char_type('#'));
            write_number_with_ubjson_prefix(j.m_data.m_value.binary->size(), true, use_bjdata);
        }

        if (use_type) {
            oa->write_characters(reinterpret_cast<const char *>(j.m_data.m_value.binary->data()),
                                 j.m_data.m_value.binary->size());
        } else {
            for (size_t i = 0; i < j.m_data.m_value.binary->size(); ++i) {
                oa->write_character(to_char_type('U'));
                oa->write_character(j.m_data.m_value.binary->data()[i]);
            }
        }

        if (!use_count) {
            oa->write_character(to_char_type(']'));
        }

        break;
    }

    case value_t::object: {
        if (use_bjdata && j.m_data.m_value.object->size() == 3
            && j.m_data.m_value.object->find("_ArrayType_") != j.m_data.m_value.object->end()
            && j.m_data.m_value.object->find("_ArraySize_") != j.m_data.m_value.object->end()
            && j.m_data.m_value.object->find("_ArrayData_") != j.m_data.m_value.object->end()) {
            if (!write_bjdata_ndarray(*j.m_data.m_value.object, use_count, use_type)) {
                break;
            }
        }

        if (add_prefix) {
            oa->write_character(to_char_type('{'));
        }

        bool prefix_required = true;
        if (use_type && !j.m_data.m_value.object->empty()) {
            const char first_prefix = ubjson_prefix(j.front(), use_bjdata);
            const bool same_prefix = std::all_of(j.begin(), j.end(),
                                                 [this, first_prefix,
                                                  use_bjdata](const ordered_json &v) {
                                                     return ubjson_prefix(v, use_bjdata)
                                                            == first_prefix;
                                                 });

            std::vector<char> bjdx = {'[', '{', 'S', 'H', 'T', 'F', 'N', 'Z'};

            if (same_prefix
                && !(use_bjdata
                     && std::find(bjdx.begin(), bjdx.end(), first_prefix) != bjdx.end())) {
                prefix_required = false;
                oa->write_character(to_char_type('$'));
                oa->write_character(first_prefix);
            }
        }

        if (use_count) {
            oa->write_character(to_char_type('#'));
            write_number_with_ubjson_prefix(j.m_data.m_value.object->size(), true, use_bjdata);
        }

        for (const auto &el : *j.m_data.m_value.object) {
            write_number_with_ubjson_prefix(el.first.size(), true, use_bjdata);
            oa->write_characters(reinterpret_cast<const char *>(el.first.c_str()), el.first.size());
            write_ubjson(el.second, use_count, use_type, prefix_required, use_bjdata);
        }

        if (!use_count) {
            oa->write_character(to_char_type('}'));
        }

        break;
    }

    case value_t::discarded:
    default: break;
    }
}

size_t nlohmann::detail::binary_writer::calc_bson_entry_header_size(const std::string &name,
                                                                    const ordered_json &j)
{
    const auto it = name.find(static_cast<typename std::string::value_type>(0));
    if (it != std::string::npos) {
        throw(
            out_of_range::create(409, concat("BSON key cannot contain code point U+0000 (at byte ",
                                             std::to_string(it), ")")));
        static_cast<void>(j);
    }

    return /*id*/ 1ul + name.size() + /*zero-terminator*/ 1u;
}

void nlohmann::detail::binary_writer::write_bson_entry_header(const std::string &name,
                                                              const uint8_t element_type)
{
    oa->write_character(to_char_type(element_type));
    oa->write_characters(reinterpret_cast<const char *>(name.c_str()), name.size() + 1u);
}

void nlohmann::detail::binary_writer::write_bson_boolean(const std::string &name, const bool value)
{
    write_bson_entry_header(name, 0x08);
    oa->write_character(value ? to_char_type(0x01) : to_char_type(0x00));
}

void nlohmann::detail::binary_writer::write_bson_double(const std::string &name, const double value)
{
    write_bson_entry_header(name, 0x01);
    write_number<double>(value, true);
}

size_t nlohmann::detail::binary_writer::calc_bson_string_size(const std::string &value)
{
    return sizeof(int32_t) + value.size() + 1ul;
}

void nlohmann::detail::binary_writer::write_bson_string(const std::string &name,
                                                        const std::string &value)
{
    write_bson_entry_header(name, 0x02);

    write_number<int32_t>(static_cast<int32_t>(value.size() + 1ul), true);
    oa->write_characters(reinterpret_cast<const char *>(value.c_str()), value.size() + 1);
}

size_t nlohmann::detail::binary_writer::calc_bson_integer_size(const int64_t value)
{
    return (std::numeric_limits<int32_t>::min)() <= value
                   && value <= (std::numeric_limits<int32_t>::max)()
               ? sizeof(int32_t)
               : sizeof(int64_t);
}

void nlohmann::detail::binary_writer::write_bson_integer(const std::string &name,
                                                         const int64_t value)
{
    if ((std::numeric_limits<int32_t>::min)() <= value
        && value <= (std::numeric_limits<int32_t>::max)()) {
        write_bson_entry_header(name, 0x10);
        write_number<int32_t>(static_cast<int32_t>(value), true);
    } else {
        write_bson_entry_header(name, 0x12);
        write_number<int64_t>(static_cast<int64_t>(value), true);
    }
}

size_t nlohmann::detail::binary_writer::calc_bson_unsigned_size(const uint64_t value) noexcept
{
    return (value <= static_cast<uint64_t>((std::numeric_limits<int32_t>::max)()))
            ? sizeof(int32_t)
            : sizeof(int64_t);
}

void nlohmann::detail::binary_writer::write_bson_object_entry(
    const std::string &name, const nlohmann::ordered_map<std::string, ordered_json> &value)
{
    write_bson_entry_header(name, 0x03);
    write_bson_object(value);
}

size_t nlohmann::detail::binary_writer::calc_bson_array_size(const std::vector<ordered_json> &value)
{
    size_t array_index = 0ul;

    const size_t embedded_document_size
        = std::accumulate(std::begin(value), std::end(value), static_cast<size_t>(0),
                          [&array_index](size_t result, const ordered_json &el) {
                              return result
                                     + calc_bson_element_size(std::to_string(array_index++), el);
                          });

    return sizeof(int32_t) + embedded_document_size + 1ul;
}

size_t nlohmann::detail::binary_writer::calc_bson_binary_size(
    const byte_container_with_subtype &value)
{
    return sizeof(int32_t) + value.size() + 1ul;
}

void nlohmann::detail::binary_writer::write_bson_array(const std::string &name,
                                                       const std::vector<ordered_json> &value)
{
    write_bson_entry_header(name, 0x04);
    write_number<int32_t>(static_cast<int32_t>(calc_bson_array_size(value)), true);

    size_t array_index = 0ul;

    for (const auto &el : value) {
        write_bson_element(std::to_string(array_index++), el);
    }

    oa->write_character(to_char_type(0x00));
}

void nlohmann::detail::binary_writer::write_bson_binary(const std::string &name,
                                                        const byte_container_with_subtype &value)
{
    write_bson_entry_header(name, 0x05);

    write_number<int32_t>(static_cast<int32_t>(value.size()), true);
    write_number(value.has_subtype() ? static_cast<uint8_t>(value.subtype())
                                     : static_cast<uint8_t>(0x00));

    oa->write_characters(reinterpret_cast<const char *>(value.data()), value.size());
}

size_t nlohmann::detail::binary_writer::calc_bson_element_size(const std::string &name,
                                                               const ordered_json &j)
{
    const auto header_size = calc_bson_entry_header_size(name, j);
    switch (j.type()) {
    case value_t::object: return header_size + calc_bson_object_size(*j.m_data.m_value.object);

    case value_t::array: return header_size + calc_bson_array_size(*j.m_data.m_value.array);

    case value_t::binary: return header_size + calc_bson_binary_size(*j.m_data.m_value.binary);

    case value_t::boolean: return header_size + 1ul;

    case value_t::number_float: return header_size + 8ul;

    case value_t::number_integer:
        return header_size + calc_bson_integer_size(j.m_data.m_value.number_integer);

    case value_t::string: return header_size + calc_bson_string_size(*j.m_data.m_value.string);

    case value_t::null: return header_size + 0ul;

    case value_t::discarded:
    default: return 0ul;
    }
}

void nlohmann::detail::binary_writer::write_bson_element(const std::string &name,
                                                         const ordered_json &j)
{
    switch (j.type()) {
    case value_t::object: return write_bson_object_entry(name, *j.m_data.m_value.object);

    case value_t::array: return write_bson_array(name, *j.m_data.m_value.array);

    case value_t::binary: return write_bson_binary(name, *j.m_data.m_value.binary);

    case value_t::boolean: return write_bson_boolean(name, j.m_data.m_value.boolean);

    case value_t::number_float: return write_bson_double(name, j.m_data.m_value.number_float);

    case value_t::number_integer: return write_bson_integer(name, j.m_data.m_value.number_integer);

    case value_t::string: return write_bson_string(name, *j.m_data.m_value.string);

    case value_t::null: return write_bson_null(name);

    case value_t::discarded:
    default: return;
    }
}

size_t nlohmann::detail::binary_writer::calc_bson_object_size(
    const nlohmann::ordered_map<std::string, ordered_json> &value)
{
    const size_t document_size = std::accumulate(
        value.begin(), value.end(), static_cast<size_t>(0),
        [](size_t result, const nlohmann::ordered_map<std::string, ordered_json>::value_type &el) {
            return result += calc_bson_element_size(el.first, el.second);
        });

    return sizeof(int32_t) + document_size + 1ul;
}

void nlohmann::detail::binary_writer::write_bson_object(const ordered_json::object_t &value)
{
    write_number<int32_t>(static_cast<int32_t>(calc_bson_object_size(value)), true);

    for (const auto &el : value) {
        write_bson_element(el.first, el.second);
    }

    oa->write_character(to_char_type(0x00));
}

void nlohmann::detail::binary_writer::write_number_with_ubjson_prefix(const double n,
                                                                      const bool add_prefix,
                                                                      const bool use_bjdata)
{
    if (add_prefix) {
        oa->write_character(get_ubjson_float_prefix(n));
    }
    write_number(n, use_bjdata);
}

void nlohmann::detail::binary_writer::write_number_with_ubjson_prefix(const size_t n,
                                                                      const bool add_prefix,
                                                                      const bool use_bjdata)
{
    return write_number_with_ubjson_prefix(int64_t(n), add_prefix, use_bjdata);
}

void nlohmann::detail::binary_writer::write_number_with_ubjson_prefix(const int64_t n,
                                                                      const bool add_prefix,
                                                                      const bool use_bjdata)
{
    if ((std::numeric_limits<std::int8_t>::min)() <= n
        && n <= (std::numeric_limits<std::int8_t>::max)()) {
        if (add_prefix) {
            oa->write_character(to_char_type('i'));
        }
        write_number(static_cast<std::int8_t>(n), use_bjdata);
    } else if (static_cast<int64_t>((std::numeric_limits<uint8_t>::min)()) <= n
               && n <= static_cast<int64_t>((std::numeric_limits<uint8_t>::max)())) {
        if (add_prefix) {
            oa->write_character(to_char_type('U'));
        }
        write_number(static_cast<uint8_t>(n), use_bjdata);
    } else if ((std::numeric_limits<std::int16_t>::min)() <= n
               && n <= (std::numeric_limits<std::int16_t>::max)()) {
        if (add_prefix) {
            oa->write_character(to_char_type('I'));
        }
        write_number(static_cast<std::int16_t>(n), use_bjdata);
    } else if (use_bjdata
               && (static_cast<int64_t>((std::numeric_limits<std::uint16_t>::min)()) <= n
                   && n <= static_cast<int64_t>((std::numeric_limits<std::uint16_t>::max)()))) {
        if (add_prefix) {
            oa->write_character(to_char_type('u'));
        }
        write_number(static_cast<uint16_t>(n), use_bjdata);
    } else if ((std::numeric_limits<int32_t>::min)() <= n
               && n <= (std::numeric_limits<int32_t>::max)()) {
        if (add_prefix) {
            oa->write_character(to_char_type('l'));
        }
        write_number(static_cast<int32_t>(n), use_bjdata);
    } else if (use_bjdata
               && (static_cast<int64_t>((std::numeric_limits<std::uint32_t>::min)()) <= n
                   && n <= static_cast<int64_t>((std::numeric_limits<std::uint32_t>::max)()))) {
        if (add_prefix) {
            oa->write_character(to_char_type('m'));
        }
        write_number(static_cast<uint32_t>(n), use_bjdata);
    } else if ((std::numeric_limits<int64_t>::min)() <= n
               && n <= (std::numeric_limits<int64_t>::max)()) {
        if (add_prefix) {
            oa->write_character(to_char_type('L'));
        }
        write_number(static_cast<int64_t>(n), use_bjdata);
    }

    else {
        if (add_prefix) {
            oa->write_character(to_char_type('H'));
        }

        const auto number = ordered_json(n).dump();
        write_number_with_ubjson_prefix(number.size(), true, use_bjdata);
        for (size_t i = 0; i < number.size(); ++i) {
            oa->write_character(to_char_type(static_cast<uint8_t>(number[i])));
        }
    }
}

bool nlohmann::detail::binary_writer::write_bjdata_ndarray(
    const nlohmann::ordered_map<std::string, ordered_json> &value, const bool use_count,
    const bool use_type)
{
    std::map<std::string, char> bjdtype = {{"uint8", 'U'},  {"int8", 'i'},   {"uint16", 'u'},
                                           {"int16", 'I'},  {"uint32", 'm'}, {"int32", 'l'},
                                           {"uint64", 'M'}, {"int64", 'L'},  {"single", 'd'},
                                           {"double", 'D'}, {"char", 'C'}};

    std::string key = "_ArrayType_";
    auto it = bjdtype.find(value.at(key).get<std::string>());
    if (it == bjdtype.end()) {
        return true;
    }
    char dtype = it->second;

    key = "_ArraySize_";
    size_t len = (value.at(key).empty() ? 0 : 1);
    for (const auto &el : value.at(key)) {
        len *= static_cast<size_t>(el.m_data.m_value.number_integer);
    }

    key = "_ArrayData_";
    if (value.at(key).size() != len) {
        return true;
    }

    oa->write_character('[');
    oa->write_character('$');
    oa->write_character(dtype);
    oa->write_character('#');

    key = "_ArraySize_";
    write_ubjson(value.at(key), use_count, use_type, true, true);

    key = "_ArrayData_";
    if (dtype == 'U' || dtype == 'C') {
        for (const auto &el : value.at(key)) {
            write_number(static_cast<uint8_t>(el.m_data.m_value.number_integer), true);
        }
    } else if (dtype == 'i') {
        for (const auto &el : value.at(key)) {
            write_number(static_cast<std::int8_t>(el.m_data.m_value.number_integer), true);
        }
    } else if (dtype == 'u') {
        for (const auto &el : value.at(key)) {
            write_number(static_cast<std::uint16_t>(el.m_data.m_value.number_integer), true);
        }
    } else if (dtype == 'I') {
        for (const auto &el : value.at(key)) {
            write_number(static_cast<std::int16_t>(el.m_data.m_value.number_integer), true);
        }
    } else if (dtype == 'm') {
        for (const auto &el : value.at(key)) {
            write_number(static_cast<std::uint32_t>(el.m_data.m_value.number_integer), true);
        }
    } else if (dtype == 'l') {
        for (const auto &el : value.at(key)) {
            write_number(static_cast<int32_t>(el.m_data.m_value.number_integer), true);
        }
    } else if (dtype == 'M') {
        for (const auto &el : value.at(key)) {
            write_number(static_cast<uint64_t>(el.m_data.m_value.number_integer), true);
        }
    } else if (dtype == 'L') {
        for (const auto &el : value.at(key)) {
            write_number(static_cast<int64_t>(el.m_data.m_value.number_integer), true);
        }
    } else if (dtype == 'd') {
        for (const auto &el : value.at(key)) {
            write_number(static_cast<float>(el.m_data.m_value.number_float), true);
        }
    } else if (dtype == 'D') {
        for (const auto &el : value.at(key)) {
            write_number(static_cast<double>(el.m_data.m_value.number_float), true);
        }
    }
    return false;
}

void nlohmann::detail::binary_writer::write_characters(const char *s, size_t length)
{
    oa->write_characters(s, length);
}

void nlohmann::detail::binary_writer::write_compact_float(const double n, input_format_t format)
{
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
#endif
    if (static_cast<double>(n) >= static_cast<double>(std::numeric_limits<float>::lowest())
        && static_cast<double>(n) <= static_cast<double>((std::numeric_limits<float>::max)())
        && static_cast<double>(static_cast<float>(n)) == static_cast<double>(n)) {
        oa->write_character(format == detail::input_format_t::cbor
                                ? get_cbor_float_prefix(static_cast<float>(n))
                                : get_msgpack_float_prefix(static_cast<float>(n)));
        write_number(static_cast<float>(n));
    } else {
        oa->write_character(format == detail::input_format_t::cbor ? get_cbor_float_prefix(n)
                                                                   : get_msgpack_float_prefix(n));
        write_number(n);
    }
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
}

char nlohmann::detail::binary_writer::ubjson_prefix(const ordered_json &j,
                                                    const bool use_bjdata) const noexcept
{
    switch (j.type()) {
    case value_t::null: return 'Z';

    case value_t::boolean: return j.m_data.m_value.boolean ? 'T' : 'F';

    case value_t::number_integer: {
        if ((std::numeric_limits<std::int8_t>::min)() <= j.m_data.m_value.number_integer
            && j.m_data.m_value.number_integer <= (std::numeric_limits<std::int8_t>::max)()) {
            return 'i';
        }
        if ((std::numeric_limits<uint8_t>::min)() <= j.m_data.m_value.number_integer
            && j.m_data.m_value.number_integer <= (std::numeric_limits<uint8_t>::max)()) {
            return 'U';
        }
        if ((std::numeric_limits<std::int16_t>::min)() <= j.m_data.m_value.number_integer
            && j.m_data.m_value.number_integer <= (std::numeric_limits<std::int16_t>::max)()) {
            return 'I';
        }
        if (use_bjdata
            && ((std::numeric_limits<std::uint16_t>::min)() <= j.m_data.m_value.number_integer
                && j.m_data.m_value.number_integer
                       <= (std::numeric_limits<std::uint16_t>::max)())) {
            return 'u';
        }
        if ((std::numeric_limits<int32_t>::min)() <= j.m_data.m_value.number_integer
            && j.m_data.m_value.number_integer <= (std::numeric_limits<int32_t>::max)()) {
            return 'l';
        }
        if (use_bjdata
            && ((std::numeric_limits<std::uint32_t>::min)() <= j.m_data.m_value.number_integer
                && j.m_data.m_value.number_integer
                       <= (std::numeric_limits<std::uint32_t>::max)())) {
            return 'm';
        }
        if ((std::numeric_limits<int64_t>::min)() <= j.m_data.m_value.number_integer
            && j.m_data.m_value.number_integer <= (std::numeric_limits<int64_t>::max)()) {
            return 'L';
        }

        return 'H';
    }

    case value_t::number_float: return get_ubjson_float_prefix(j.m_data.m_value.number_float);

    case value_t::string: return 'S';

    case value_t::array:
    case value_t::binary: return '[';

    case value_t::object: return '{';

    case value_t::discarded:
    default: return 'N';
    }
}
