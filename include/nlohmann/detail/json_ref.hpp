//     __ _____ _____ _____
//  __|  |   __|     |   | |  JSON for Modern C++
// |  |  |__   |  |  | | | |  version 3.11.3
// |_____|_____|_____|_|___|  https://github.com/nlohmann/json
//
// SPDX-FileCopyrightText: 2013-2023 Niels Lohmann <https://nlohmann.me>
// SPDX-License-Identifier: MIT

#pragma once

#include <initializer_list>
#include <utility>

namespace nlohmann {
namespace detail {

template<typename T> class json_ref
{
public:
    using value_type = T;

    json_ref(value_type &&value) : owned_value(std::move(value)) {}
    json_ref(const value_type &value) : value_ref(&value) {}
    json_ref(std::initializer_list<json_ref> init) : owned_value(init) {}

    template<class... Args,
             std::enable_if_t<std::is_constructible<value_type, Args...>::value, int> = 0>
    json_ref(Args &&...args) : owned_value(std::forward<Args>(args)...)
    {}

    json_ref(json_ref &&) noexcept = default;
    json_ref(const json_ref &) = delete;
    json_ref &operator=(const json_ref &) = delete;
    json_ref &operator=(json_ref &&) = delete;
    ~json_ref() = default;

    value_type moved_or_copied() const
    {
        if (value_ref == nullptr) {
            return std::move(owned_value);
        }
        return *value_ref;
    }

    value_type const &operator*() const { return value_ref ? *value_ref : owned_value; }
    value_type const *operator->() const { return &**this; }

private:
    mutable value_type owned_value = nullptr;
    value_type const *value_ref = nullptr;
};

} // namespace detail
} // namespace nlohmann
