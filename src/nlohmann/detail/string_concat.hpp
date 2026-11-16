//     __ _____ _____ _____
//  __|  |   __|     |   | |  JSON for Modern C++
// |  |  |__   |  |  | | | |  version 3.11.3
// |_____|_____|_____|_|___|  https://github.com/nlohmann/json
//
// SPDX-FileCopyrightText: 2013-2023 Niels Lohmann <https://nlohmann.me>
// SPDX-License-Identifier: MIT

#pragma once

#include <cstring>
#include <string>
#include <utility>

namespace nlohmann {
namespace detail {

inline size_t concat_length() { return 0; }

template<typename... Args> inline size_t concat_length(const char &c, const Args &...rest);
template<typename... Args> inline size_t concat_length(const char *cstr, const Args &...rest);
template<typename... Args>
inline size_t concat_length(const std::string &str, const Args &...rest);

template<typename... Args> inline size_t concat_length(const char &c, const Args &...rest)
{
    return 1 + concat_length(rest...);
}

template<typename... Args> inline size_t concat_length(const char *cstr, const Args &...rest)
{
    return strlen(cstr) + concat_length(rest...);
}

template<typename... Args> inline size_t concat_length(const std::string &str, const Args &...rest)
{
    return str.size() + concat_length(rest...);
}

inline void concat_into(std::string & /*out*/) {}

template<typename Arg, typename... Args>
inline void concat_into(std::string &out, Arg &&arg, Args &&...rest)
{
    out += std::forward<Arg>(arg);
    concat_into(out, std::forward<Args>(rest)...);
}

template<typename... Args> inline std::string concat(Args &&...args)
{
    std::string str;
    str.reserve(concat_length(args...));
    concat_into(str, std::forward<Args>(args)...);
    return str;
}

} // namespace detail
} // namespace nlohmann
