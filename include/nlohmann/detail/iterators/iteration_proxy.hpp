//     __ _____ _____ _____
//  __|  |   __|     |   | |  JSON for Modern C++
// |  |  |__   |  |  | | | |  version 3.11.3
// |_____|_____|_____|_|___|  https://github.com/nlohmann/json
//
// SPDX-FileCopyrightText: 2013-2023 Niels Lohmann <https://nlohmann.me>
// SPDX-License-Identifier: MIT

#pragma once

#include <cstddef>
#include <iterator>
#include <string>
#include <tuple>
#include <utility>

#include "nlohmann/detail/value_t.hpp"

namespace nlohmann {
namespace detail {

template<typename string_type> void int_to_string(string_type &target, std::size_t value)
{
    using std::to_string;
    target = to_string(value);
}
template<typename IteratorType> class iteration_proxy_value
{
public:
    using difference_type = std::ptrdiff_t;
    using value_type = iteration_proxy_value;
    using pointer = value_type *;
    using reference = value_type &;
    using iterator_category = std::input_iterator_tag;
    using string_type = typename std::remove_cv<
        typename std::remove_reference<decltype(std::declval<IteratorType>().key())>::type>::type;

private:
    IteratorType anchor{};
    std::size_t array_index = 0;
    mutable std::size_t array_index_last = 0;
    mutable string_type array_index_str = "0";
    string_type empty_str{};

public:
    explicit iteration_proxy_value() = default;
    explicit iteration_proxy_value(IteratorType it, std::size_t array_index_ = 0) noexcept(
        std::is_nothrow_move_constructible<IteratorType>::value
            &&std::is_nothrow_default_constructible<string_type>::value)
        : anchor(std::move(it)), array_index(array_index_)
    {}

    iteration_proxy_value(iteration_proxy_value const &) = default;
    iteration_proxy_value &operator=(iteration_proxy_value const &) = default;

    iteration_proxy_value(iteration_proxy_value &&) noexcept(
        std::is_nothrow_move_constructible<IteratorType>::value
            &&std::is_nothrow_move_constructible<string_type>::value)
        = default;
    iteration_proxy_value &operator=(iteration_proxy_value &&) noexcept(
        std::is_nothrow_move_assignable<IteratorType>::value
            &&std::is_nothrow_move_assignable<string_type>::value)
        = default;
    ~iteration_proxy_value() = default;

    const iteration_proxy_value &operator*() const { return *this; }

    iteration_proxy_value &operator++()
    {
        ++anchor;
        ++array_index;

        return *this;
    }

    iteration_proxy_value operator++(int) &
    {
        auto tmp = iteration_proxy_value(anchor, array_index);
        ++anchor;
        ++array_index;
        return tmp;
    }

    bool operator==(const iteration_proxy_value &o) const { return anchor == o.anchor; }

    bool operator!=(const iteration_proxy_value &o) const { return anchor != o.anchor; }

    const string_type &key() const
    {
        switch (anchor.m_object->type()) {
        case value_t::array: {
            if (array_index != array_index_last) {
                int_to_string(array_index_str, array_index);
                array_index_last = array_index;
            }
            return array_index_str;
        }

        case value_t::object: return anchor.key();

        case value_t::null:
        case value_t::string:
        case value_t::boolean:
        case value_t::number_integer:
        case value_t::number_float:
        case value_t::binary:
        case value_t::discarded:
        default: return empty_str;
        }
    }

    typename IteratorType::reference value() const { return anchor.value(); }
};

template<typename IteratorType> class iteration_proxy
{
private:
    typename IteratorType::pointer container = nullptr;

public:
    explicit iteration_proxy() = default;

    explicit iteration_proxy(typename IteratorType::reference cont) noexcept : container(&cont) {}

    iteration_proxy(iteration_proxy const &) = default;
    iteration_proxy &operator=(iteration_proxy const &) = default;
    iteration_proxy(iteration_proxy &&) noexcept = default;
    iteration_proxy &operator=(iteration_proxy &&) noexcept = default;
    ~iteration_proxy() = default;

    iteration_proxy_value<IteratorType> begin() const noexcept
    {
        return iteration_proxy_value<IteratorType>(container->begin());
    }

    iteration_proxy_value<IteratorType> end() const noexcept
    {
        return iteration_proxy_value<IteratorType>(container->end());
    }
};

template<std::size_t N, typename IteratorType, std::enable_if_t<N == 0, int> = 0>
auto get(const nlohmann::detail::iteration_proxy_value<IteratorType> &i) -> decltype(i.key())
{
    return i.key();
}

template<std::size_t N, typename IteratorType, std::enable_if_t<N == 1, int> = 0>
auto get(const nlohmann::detail::iteration_proxy_value<IteratorType> &i) -> decltype(i.value())
{
    return i.value();
}

} // namespace detail
} // namespace nlohmann

namespace std {

#if defined(__clang__)

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmismatched-tags"
#endif
template<typename IteratorType>
class tuple_size<::nlohmann::detail::iteration_proxy_value<IteratorType>>
    : public std::integral_constant<std::size_t, 2>
{};

template<std::size_t N, typename IteratorType>
class tuple_element<N, ::nlohmann::detail::iteration_proxy_value<IteratorType>>
{
public:
    using type = decltype(get<N>(
        std::declval<::nlohmann::detail::iteration_proxy_value<IteratorType>>()));
};
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

} // namespace std
