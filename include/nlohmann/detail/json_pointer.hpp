//     __ _____ _____ _____
//  __|  |   __|     |   | |  JSON for Modern C++
// |  |  |__   |  |  | | | |  version 3.11.3
// |_____|_____|_____|_|___|  https://github.com/nlohmann/json
//
// SPDX-FileCopyrightText: 2013-2023 Niels Lohmann <https://nlohmann.me>
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdlib>
#include <string>
#include <utility>
#include <vector>

namespace nlohmann {
class ordered_json;

class json_pointer
{
    friend class ordered_json;

    template<typename T> struct string_t_helper
    {
        using type = T;
    };

public:
    explicit json_pointer(const std::string &s = "") : reference_tokens(split(s)) {}
    std::string to_string() const;
    json_pointer &operator/=(const json_pointer &ptr);
    json_pointer &operator/=(std::string token);
    json_pointer &operator/=(size_t array_idx) { return *this /= std::to_string(array_idx); }

    friend json_pointer operator/(const json_pointer &lhs, const json_pointer &rhs)
    {
        return json_pointer(lhs) /= rhs;
    }
    friend json_pointer operator/(const json_pointer &lhs, std::string token)
    {
        return json_pointer(lhs) /= std::move(token);
    }
    friend json_pointer operator/(const json_pointer &lhs, size_t array_idx)
    {
        return json_pointer(lhs) /= array_idx;
    }

    json_pointer parent_pointer() const;
    void pop_back();
    const std::string &back() const;
    void push_back(const std::string &token) { reference_tokens.push_back(token); }
    void push_back(std::string &&token) { reference_tokens.push_back(std::move(token)); }
    bool empty() const noexcept { return reference_tokens.empty(); }

#ifndef JSON_TESTS_PRIVATE
private:
#endif
    json_pointer top() const;

private:
    static size_t array_index(const std::string &s);

    ordered_json &get_and_create(ordered_json &j) const;
    ordered_json &get_unchecked(ordered_json *ptr) const;
    ordered_json &get_checked(ordered_json *ptr) const;
    const ordered_json &get_unchecked(const ordered_json *ptr) const;
    const ordered_json &get_checked(const ordered_json *ptr) const;
    bool contains(const ordered_json *ptr) const;
    static std::vector<std::string> split(const std::string &reference_string);

    static void flatten(const std::string &reference_string, const ordered_json &value,
                        ordered_json &result);
    static ordered_json unflatten(const ordered_json &value);

public:
    bool operator==(const json_pointer &rhs) const noexcept;
    std::strong_ordering operator<=>(const json_pointer &rhs) const noexcept;

private:
    std::vector<std::string> reference_tokens;
};

} // namespace nlohmann
