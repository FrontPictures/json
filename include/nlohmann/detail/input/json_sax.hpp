//     __ _____ _____ _____
//  __|  |   __|     |   | |  JSON for Modern C++
// |  |  |__   |  |  | | | |  version 3.11.3
// |_____|_____|_____|_|___|  https://github.com/nlohmann/json
//
// SPDX-FileCopyrightText: 2013-2023 Niels Lohmann <https://nlohmann.me>
// SPDX-License-Identifier: MIT

#pragma once

#include <cstddef>
#include <string>
#include <utility>
#include <vector>
#include <memory>

#include "nlohmann/detail/exceptions.hpp"
#include "parser_types.h"

namespace nlohmann {
class byte_container_with_subtype;

class json_sax
{
public:
    virtual bool null() = 0;
    virtual bool boolean(bool val) = 0;
    virtual bool number_integer(int64_t val) = 0;
    virtual bool number_float(double val, const std::string &s) = 0;
    virtual bool string(std::string &val) = 0;
    virtual bool binary(byte_container_with_subtype &val) = 0;
    virtual bool start_object(size_t elements) = 0;
    virtual bool key(std::string &val) = 0;
    virtual bool end_object() = 0;
    virtual bool start_array(size_t elements) = 0;
    virtual bool end_array() = 0;
    virtual bool parse_error(size_t position, const std::string &last_token,
                             const detail::exception &ex)
        = 0;

    json_sax() = default;
    json_sax(const json_sax &) = default;
    json_sax(json_sax &&) noexcept = default;
    json_sax &operator=(const json_sax &) = default;
    json_sax &operator=(json_sax &&) noexcept = default;
    virtual ~json_sax() = default;
};

namespace detail {
class json_sax_dom_parser : public json_sax
{
public:
    explicit json_sax_dom_parser(ordered_json &r, const bool allow_exceptions_ = true);
    ~json_sax_dom_parser() = default;

    json_sax_dom_parser &operator=(const json_sax_dom_parser &) = delete;
    json_sax_dom_parser &operator=(json_sax_dom_parser &&) = delete;

    bool null();
    bool boolean(bool val);
    bool number_integer(int64_t val);
    bool number_float(double val, const std::string &);
    bool string(std::string &val);
    bool binary(byte_container_with_subtype &val);
    bool start_object(size_t len);
    bool key(std::string &val);
    bool end_object();
    bool start_array(size_t len);
    bool end_array();
    bool parse_error(size_t, const std::string &, const detail::exception &ex);

    constexpr bool is_errored() const { return errored; }

private:
    ordered_json *handle_value(ordered_json &&v);
    ordered_json &root;
    std::vector<ordered_json *> ref_stack{};
    ordered_json *object_element = nullptr;

    bool errored = false;
    const bool allow_exceptions = true;
};

class json_sax_dom_callback_parser : public json_sax
{
public:
    json_sax_dom_callback_parser(ordered_json &r, const parser_callback_t cb,
                                 const bool allow_exceptions_ = true);
    ~json_sax_dom_callback_parser();

    bool null();
    bool boolean(bool val);
    bool number_integer(int64_t val);
    bool number_float(double val, const std::string &);
    bool string(std::string &val);
    bool binary(byte_container_with_subtype &val);
    bool start_object(size_t len);
    bool key(std::string &val);
    bool end_object();
    bool start_array(size_t len);
    bool end_array();
    bool parse_error(size_t, const std::string &, const detail::exception &ex);

    constexpr bool is_errored() const { return errored; }

private:
    std::pair<bool, ordered_json *> handle_value(ordered_json &&v,
                                                 const bool skip_callback = false);

    ordered_json &root;
    std::vector<ordered_json *> ref_stack{};
    std::vector<bool> keep_stack{};
    std::vector<bool> key_keep_stack{};
    ordered_json *object_element = nullptr;
    bool errored = false;
    const parser_callback_t callback = nullptr;
    const bool allow_exceptions = true;
    std::unique_ptr<ordered_json> discarded;
};

class json_sax_acceptor : public json_sax
{
public:
    bool null() { return true; }
    bool boolean(bool) { return true; }
    bool number_integer(int64_t) { return true; }
    bool number_float(double, const std::string &) { return true; }
    bool string(std::string &) { return true; }
    bool binary(byte_container_with_subtype &) { return true; }
    bool start_object(size_t = static_cast<size_t>(-1)) { return true; }
    bool key(std::string &) { return true; }
    bool end_object() { return true; }
    bool start_array(size_t = static_cast<size_t>(-1)) { return true; }
    bool end_array() { return true; }
    bool parse_error(size_t, const std::string &, const detail::exception &) { return false; }
};

} // namespace detail
} // namespace nlohmann
