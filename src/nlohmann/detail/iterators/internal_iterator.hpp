//     __ _____ _____ _____
//  __|  |   __|     |   | |  JSON for Modern C++
// |  |  |__   |  |  | | | |  version 3.11.3
// |_____|_____|_____|_|___|  https://github.com/nlohmann/json
//
// SPDX-FileCopyrightText: 2013-2023 Niels Lohmann <https://nlohmann.me>
// SPDX-License-Identifier: MIT

#pragma once

#include <vector>
#include "nlohmann/detail/iterators/primitive_iterator.hpp"
#include "nlohmann/ordered_map.hpp"

namespace nlohmann {
class ordered_json;
namespace detail {

template<typename T> class internal_iterator
{
public:
    typename nlohmann::ordered_map<std::string, T>::iterator object_iterator{};
    typename std::vector<T>::iterator array_iterator{};
    primitive_iterator_t primitive_iterator{};
};

} // namespace detail
} // namespace nlohmann
