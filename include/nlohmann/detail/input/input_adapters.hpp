//     __ _____ _____ _____
//  __|  |   __|     |   | |  JSON for Modern C++
// |  |  |__   |  |  | | | |  version 3.11.3
// |_____|_____|_____|_|___|  https://github.com/nlohmann/json
//
// SPDX-FileCopyrightText: 2013-2023 Niels Lohmann <https://nlohmann.me>
// SPDX-License-Identifier: MIT

#pragma once

#include <string>
#include <vector>

namespace nlohmann {
namespace detail {

class iterator_input_adapter
{
public:
    iterator_input_adapter(std::string::const_iterator first, std::string::const_iterator last);
    iterator_input_adapter(std::vector<uint8_t>::const_iterator first,
                           std::vector<uint8_t>::const_iterator last);

    uint64_t get_character();

private:
    std::string::const_iterator current;
    std::string::const_iterator end;
    std::vector<uint8_t>::const_iterator vcurrent;
    std::vector<uint8_t>::const_iterator vend;
    bool vectorType = false;
};

class iterator_input_adapter_factory
{
public:
    static iterator_input_adapter create(std::string::const_iterator first,
                                         std::string::const_iterator last);
    static iterator_input_adapter create(std::vector<uint8_t>::const_iterator first,
                                         std::vector<uint8_t>::const_iterator last);
};

namespace container_input_adapter_factory_impl {

class container_input_adapter_factory
{
public:
    static iterator_input_adapter create(const std::string &container);
    static iterator_input_adapter create(const std::vector<uint8_t> &container);
};
} // namespace container_input_adapter_factory_impl

iterator_input_adapter input_adapter(const std::string &container);
iterator_input_adapter input_adapter(const std::vector<uint8_t> &container);
} // namespace detail
} // namespace nlohmann
