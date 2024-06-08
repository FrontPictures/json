//     __ _____ _____ _____
//  __|  |   __|     |   | |  JSON for Modern C++
// |  |  |__   |  |  | | | |  version 3.11.3
// |_____|_____|_____|_|___|  https://github.com/nlohmann/json
//
// SPDX-FileCopyrightText: 2013-2023 Niels Lohmann <https://nlohmann.me>
// SPDX-License-Identifier: MIT

#pragma once

#include <iterator>
#include <type_traits>

#include "nlohmann/detail/exceptions.hpp"
#include "nlohmann/detail/iterators/internal_iterator.hpp"
#include "nlohmann/detail/iterators/primitive_iterator.hpp"
#include "nlohmann/detail/value_t.hpp"

namespace nlohmann {
namespace detail {

template<typename IteratorType> class iteration_proxy;
template<typename IteratorType> class iteration_proxy_value;

template<typename T> class iter_impl
{
    friend T;
    friend iteration_proxy<iter_impl>;
    friend iteration_proxy_value<iter_impl>;
    friend class iter_impl<const T>;

    using object_t = nlohmann::ordered_map<std::string, T>;

public:
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = typename T::value_type;
    using difference_type = typename T::difference_type;
    using pointer = typename std::conditional<std::is_const<T>::value, typename T::const_pointer,
                                              typename T::pointer>::type;
    using reference = typename std::conditional<
        std::is_const<T>::value, typename T::const_reference, typename T::reference>::type;

    iter_impl() = default;
    ~iter_impl() = default;
    iter_impl(iter_impl &&) noexcept = default;
    iter_impl &operator=(iter_impl &&) noexcept = default;

    explicit iter_impl(pointer object) noexcept : m_object(object)
    {
        switch (m_object->m_data.m_type) {
        case value_t::object: {
            m_it.object_iterator = {};
            break;
        }

        case value_t::array: {
            m_it.array_iterator = {};
            break;
        }

        case value_t::null:
        case value_t::string:
        case value_t::boolean:
        case value_t::number_integer:
        case value_t::number_float:
        case value_t::binary:
        case value_t::discarded:
        default: {
            m_it.primitive_iterator = primitive_iterator_t();
            break;
        }
        }
    }

    iter_impl(const iter_impl<const T> &other) noexcept
        : m_object(other.m_object), m_it(other.m_it)
    {}

    iter_impl &operator=(const iter_impl<const T> &other) noexcept
    {
        if (&other != this) {
            m_object = other.m_object;
            m_it = other.m_it;
        }
        return *this;
    }

    iter_impl(const iter_impl<typename std::remove_const<T>::type> &other) noexcept
        : m_object(other.m_object), m_it(other.m_it)
    {}

    iter_impl &operator=(const iter_impl<typename std::remove_const<T>::type> &other) noexcept
    {
        m_object = other.m_object;
        m_it = other.m_it;
        return *this;
    }
#ifndef JSON_TESTS_PRIVATE
private:
#endif
    void set_begin() noexcept
    {
        switch (m_object->m_data.m_type) {
        case value_t::object: {
            m_it.object_iterator = m_object->m_data.m_value.object->begin();
            break;
        }

        case value_t::array: {
            m_it.array_iterator = m_object->m_data.m_value.array->begin();
            break;
        }

        case value_t::null: {
            m_it.primitive_iterator.set_end();
            break;
        }

        case value_t::string:
        case value_t::boolean:
        case value_t::number_integer:
        case value_t::number_float:
        case value_t::binary:
        case value_t::discarded:
        default: {
            m_it.primitive_iterator.set_begin();
            break;
        }
        }
    }

    void set_end() noexcept
    {
        switch (m_object->m_data.m_type) {
        case value_t::object: {
            m_it.object_iterator = m_object->m_data.m_value.object->end();
            break;
        }

        case value_t::array: {
            m_it.array_iterator = m_object->m_data.m_value.array->end();
            break;
        }

        case value_t::null:
        case value_t::string:
        case value_t::boolean:
        case value_t::number_integer:
        case value_t::number_float:
        case value_t::binary:
        case value_t::discarded:
        default: {
            m_it.primitive_iterator.set_end();
            break;
        }
        }
    }

public:
    reference operator*() const
    {
        switch (m_object->m_data.m_type) {
        case value_t::object: {
            return m_it.object_iterator->second;
        }

        case value_t::array: {
            return *m_it.array_iterator;
        }

        case value_t::null: throw(invalid_iterator::create(214, "cannot get value"));

        case value_t::string:
        case value_t::boolean:
        case value_t::number_integer:
        case value_t::number_float:
        case value_t::binary:
        case value_t::discarded:
        default: {
            if (m_it.primitive_iterator.is_begin()) {
                return *m_object;
            }

            throw(invalid_iterator::create(214, "cannot get value"));
        }
        }
    }

    pointer operator->() const
    {
        switch (m_object->m_data.m_type) {
        case value_t::object: {
            return &(m_it.object_iterator->second);
        }

        case value_t::array: {
            return &*m_it.array_iterator;
        }

        case value_t::null:
        case value_t::string:
        case value_t::boolean:
        case value_t::number_integer:
        case value_t::number_float:
        case value_t::binary:
        case value_t::discarded:
        default: {
            if (m_it.primitive_iterator.is_begin()) {
                return m_object;
            }

            throw(invalid_iterator::create(214, "cannot get value"));
        }
        }
    }

    iter_impl operator++(int) &
    {
        auto result = *this;
        ++(*this);
        return result;
    }

    iter_impl &operator++()
    {
        switch (m_object->m_data.m_type) {
        case value_t::object: {
            std::advance(m_it.object_iterator, 1);
            break;
        }

        case value_t::array: {
            std::advance(m_it.array_iterator, 1);
            break;
        }

        case value_t::null:
        case value_t::string:
        case value_t::boolean:
        case value_t::number_integer:
        case value_t::number_float:
        case value_t::binary:
        case value_t::discarded:
        default: {
            ++m_it.primitive_iterator;
            break;
        }
        }

        return *this;
    }

    iter_impl operator--(int) &
    {
        auto result = *this;
        --(*this);
        return result;
    }

    iter_impl &operator--()
    {
        switch (m_object->m_data.m_type) {
        case value_t::object: {
            std::advance(m_it.object_iterator, -1);
            break;
        }

        case value_t::array: {
            std::advance(m_it.array_iterator, -1);
            break;
        }

        case value_t::null:
        case value_t::string:
        case value_t::boolean:
        case value_t::number_integer:
        case value_t::number_float:
        case value_t::binary:
        case value_t::discarded:
        default: {
            --m_it.primitive_iterator;
            break;
        }
        }

        return *this;
    }

    template<typename IterImpl> bool operator==(const IterImpl &other) const
    {
        if (m_object != other.m_object) {
            throw(
                invalid_iterator::create(212, "cannot compare iterators of different containers"));
        }

        switch (m_object->m_data.m_type) {
        case value_t::object: return (m_it.object_iterator == other.m_it.object_iterator);

        case value_t::array: return (m_it.array_iterator == other.m_it.array_iterator);

        case value_t::null:
        case value_t::string:
        case value_t::boolean:
        case value_t::number_integer:
        case value_t::number_float:
        case value_t::binary:
        case value_t::discarded:
        default: return (m_it.primitive_iterator == other.m_it.primitive_iterator);
        }
    }

    template<typename IterImpl> bool operator!=(const IterImpl &other) const
    {
        return !operator==(other);
    }

    bool operator<(const iter_impl &other) const
    {
        if (m_object != other.m_object) {
            throw(invalid_iterator::create(212, "cannot compare iterators of different containers"));
        }

        switch (m_object->m_data.m_type) {
        case value_t::object:
            throw(invalid_iterator::create(213, "cannot compare order of object iterators"));

        case value_t::array: return (m_it.array_iterator < other.m_it.array_iterator);

        case value_t::null:
        case value_t::string:
        case value_t::boolean:
        case value_t::number_integer:
        case value_t::number_float:
        case value_t::binary:
        case value_t::discarded:
        default: return (m_it.primitive_iterator < other.m_it.primitive_iterator);
        }
    }

    bool operator<=(const iter_impl &other) const { return !other.operator<(*this); }
    bool operator>(const iter_impl &other) const { return !operator<=(other); }
    bool operator>=(const iter_impl &other) const { return !operator<(other); }

    iter_impl &operator+=(difference_type i)
    {
        switch (m_object->m_data.m_type) {
        case value_t::object:
            throw(invalid_iterator::create(209, "cannot use offsets with object iterators"));

        case value_t::array: {
            std::advance(m_it.array_iterator, i);
            break;
        }

        case value_t::null:
        case value_t::string:
        case value_t::boolean:
        case value_t::number_integer:
        case value_t::number_float:
        case value_t::binary:
        case value_t::discarded:
        default: {
            m_it.primitive_iterator += i;
            break;
        }
        }

        return *this;
    }

    iter_impl &operator-=(difference_type i) { return operator+=(-i); }

    iter_impl operator+(difference_type i) const
    {
        auto result = *this;
        result += i;
        return result;
    }

    friend iter_impl operator+(difference_type i, const iter_impl &it)
    {
        auto result = it;
        result += i;
        return result;
    }

    iter_impl operator-(difference_type i) const
    {
        auto result = *this;
        result -= i;
        return result;
    }

    difference_type operator-(const iter_impl &other) const
    {
        switch (m_object->m_data.m_type) {
        case value_t::object:
            throw(invalid_iterator::create(209, "cannot use offsets with object iterators"));

        case value_t::array: return m_it.array_iterator - other.m_it.array_iterator;

        case value_t::null:
        case value_t::string:
        case value_t::boolean:
        case value_t::number_integer:
        case value_t::number_float:
        case value_t::binary:
        case value_t::discarded:
        default: return m_it.primitive_iterator - other.m_it.primitive_iterator;
        }
    }

    reference operator[](difference_type n) const
    {
        switch (m_object->m_data.m_type) {
        case value_t::object:
            throw(invalid_iterator::create(208, "cannot use operator[] for object iterators"));

        case value_t::array: return *std::next(m_it.array_iterator, n);

        case value_t::null: throw(invalid_iterator::create(214, "cannot get value"));

        case value_t::string:
        case value_t::boolean:
        case value_t::number_integer:
        case value_t::number_float:
        case value_t::binary:
        case value_t::discarded:
        default: {
            if (m_it.primitive_iterator.get_value() == -n) {
                return *m_object;
            }

            throw(invalid_iterator::create(214, "cannot get value"));
        }
        }
    }

    const std::string &key() const
    {
        if (m_object->is_object()) {
            return m_it.object_iterator->first;
        }

        throw(invalid_iterator::create(207, "cannot use key() for non-object iterators"));
    }

    reference value() const { return operator*(); }

#ifndef JSON_TESTS_PRIVATE
private:
#endif
    pointer m_object = nullptr;
    internal_iterator<typename std::remove_const<T>::type> m_it{};
};

} // namespace detail
} // namespace nlohmann
