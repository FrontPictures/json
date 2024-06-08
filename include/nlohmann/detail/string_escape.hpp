//     __ _____ _____ _____
//  __|  |   __|     |   | |  JSON for Modern C++
// |  |  |__   |  |  | | | |  version 3.11.3
// |_____|_____|_____|_|___|  https://github.com/nlohmann/json
//
// SPDX-FileCopyrightText: 2013-2023 Niels Lohmann <https://nlohmann.me>
// SPDX-License-Identifier: MIT

#pragma once

#include <string>

namespace nlohmann {
namespace detail {

inline void replace_substring(std::string &s, const std::string &f, const std::string &t)
{
    for (auto pos = s.find(f); pos != std::string::npos;
         s.replace(pos, f.size(), t), pos = s.find(f, pos + t.size())) {}
}

inline std::string escape(std::string s)
{
    replace_substring(s, std::string{"~"}, std::string{"~0"});
    replace_substring(s, std::string{"/"}, std::string{"~1"});
    return s;
}

static void unescape(std::string &s)
{
    replace_substring(s, std::string{"~1"}, std::string{"/"});
    replace_substring(s, std::string{"~0"}, std::string{"~"});
}

} // namespace detail
} // namespace nlohmann
