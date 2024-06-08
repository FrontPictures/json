//     __ _____ _____ _____
//  __|  |   __|     |   | |  JSON for Modern C++
// |  |  |__   |  |  | | | |  version 3.11.3
// |_____|_____|_____|_|___|  https://github.com/nlohmann/json
//
// SPDX-FileCopyrightText: 2013-2023 Niels Lohmann <https://nlohmann.me>
// SPDX-License-Identifier: MIT

#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace nlohmann {
namespace detail {

class output_adapter_protocol
{
public:
    virtual void write_character(char c) = 0;
    virtual void write_characters(const char *s, size_t length) = 0;
    virtual ~output_adapter_protocol() = default;

    output_adapter_protocol() = default;
    output_adapter_protocol(const output_adapter_protocol &) = default;
    output_adapter_protocol(output_adapter_protocol &&) noexcept = default;
    output_adapter_protocol &operator=(const output_adapter_protocol &) = default;
    output_adapter_protocol &operator=(output_adapter_protocol &&) noexcept = default;
};

class output_vector_adapter : public output_adapter_protocol
{
public:
    explicit output_vector_adapter(std::vector<uint8_t> &vec) noexcept : v(vec) {}
    void write_character(char c) override { v.push_back(c); }
    void write_characters(const char *s, size_t l) override { v.insert(v.end(), s, s + l); }

private:
    std::vector<uint8_t> &v;
};

class output_string_adapter : public output_adapter_protocol
{
public:
    explicit output_string_adapter(std::string &s) noexcept : str(s) {}
    void write_character(char c) override { str.push_back(c); }
    void write_characters(const char *s, size_t length) override { str.append(s, length); }

private:
    std::string &str;
};

class output_adapter
{
public:
    output_adapter(std::vector<uint8_t> &vec) : oa(std::make_shared<output_vector_adapter>(vec)) {}
    output_adapter(std::string &s) : oa(std::make_shared<output_string_adapter>(s)) {}
    operator std::shared_ptr<output_adapter_protocol>() const { return oa; }

private:
    std::shared_ptr<output_adapter_protocol> oa;
};

} // namespace detail
} // namespace nlohmann
