//     __ _____ _____ _____
//  __|  |   __|     |   | |  JSON for Modern C++ (supporting code)
// |  |  |__   |  |  | | | |  version 3.11.3
// |_____|_____|_____|_|___|  https://github.com/nlohmann/json
//
// SPDX-FileCopyrightText: 2013-2023 Niels Lohmann <https://nlohmann.me>
// SPDX-License-Identifier: MIT

#pragma once

#include <nlohmann/json.hpp>
#include <map>
#include <cstdint> // uint8_t
#include <fstream> // ifstream, istreambuf_iterator, ios
#include <vector> // vector

namespace utils
{
// External JSON specifications treat object order as insignificant. Normalize
// only their test fixtures; the library itself retains insertion order.
inline nlohmann::json sort_objects(const nlohmann::json &value)
{
    if (value.is_object()) {
        std::map<std::string, nlohmann::json> sorted;
        for (auto it = value.begin(); it != value.end(); ++it) {
            sorted.emplace(it.key(), sort_objects(it.value()));
        }
        auto result = nlohmann::json::object();
        for (const auto &item : sorted) {
            result[item.first] = item.second;
        }
        return result;
    }
    if (value.is_array()) {
        auto result = nlohmann::json::array();
        for (const auto &item : value) {
            result.push_back(sort_objects(item));
        }
        return result;
    }
    return value;
}
inline bool same_json_values(const nlohmann::json &a, const nlohmann::json &b)
{
    return sort_objects(a) == sort_objects(b);
}


inline std::vector<std::uint8_t> read_binary_file(const std::string& filename)
{
    std::ifstream file(filename, std::ios::binary);
    file.unsetf(std::ios::skipws);

    file.seekg(0, std::ios::end);
    const auto size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<std::uint8_t> byte_vector;
    byte_vector.reserve(static_cast<std::size_t>(size));
    byte_vector.insert(byte_vector.begin(), std::istream_iterator<std::uint8_t>(file), std::istream_iterator<std::uint8_t>());
    return byte_vector;
}

} // namespace utils
