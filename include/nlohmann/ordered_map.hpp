//     __ _____ _____ _____
//  __|  |   __|     |   | |  JSON for Modern C++
// |  |  |__   |  |  | | | |  version 3.11.3
// |_____|_____|_____|_|___|  https://github.com/nlohmann/json
//
// SPDX-FileCopyrightText: 2013-2023 Niels Lohmann <https://nlohmann.me>
// SPDX-License-Identifier: MIT

#pragma once

#include <functional>
#include <initializer_list>
#include <iterator>
#include <utility>
#include <vector>
#include<stdexcept>

namespace nlohmann {

template<class Key, class T> struct ordered_map : std::vector<std::pair<const Key, T>>
{
    using key_type = Key;
    using mapped_type = T;
    using Container = std::vector<std::pair<const Key, T>>;
    using iterator = typename Container::iterator;
    using const_iterator = typename Container::const_iterator;
    using value_type = typename Container::value_type;
    using key_compare = std::equal_to<>;

    ordered_map() noexcept(noexcept(Container())) : Container{} {}
    template<class It> ordered_map(It first, It last) : Container{first, last} {}
    ordered_map(std::initializer_list<value_type> init) : Container{init} {}

    std::pair<iterator, bool> emplace(const key_type &key, T &&t)
    {
        for (auto it = this->begin(); it != this->end(); ++it) {
            if (m_compare(it->first, key)) {
                return {it, false};
            }
        }
        Container::emplace_back(key, std::forward<T>(t));
        return {std::prev(this->end()), true};
    }

    std::pair<iterator, bool> emplace(Key &&key, T &&t)
    {
        for (auto it = this->begin(); it != this->end(); ++it) {
            if (m_compare(it->first, key)) {
                return {it, false};
            }
        }
        Container::emplace_back(std::forward<Key>(key), std::forward<T>(t));
        return {std::prev(this->end()), true};
    }

    T &operator[](const key_type &key) { return emplace(key, T{}).first->second; }

    T &operator[](Key &&key) { return emplace(std::forward<Key>(key), T{}).first->second; }

    const T &operator[](const key_type &key) const { return at(key); }

    const T &operator[](Key &&key) const { return at(std::forward<Key>(key)); }

    T &at(const key_type &key)
    {
        for (auto it = this->begin(); it != this->end(); ++it) {
            if (m_compare(it->first, key)) {
                return it->second;
            }
        }

        throw(std::out_of_range("key not found"));
    }

    T &at(Key &&key)
    {
        for (auto it = this->begin(); it != this->end(); ++it) {
            if (m_compare(it->first, key)) {
                return it->second;
            }
        }

        throw(std::out_of_range("key not found"));
    }

    const T &at(const key_type &key) const
    {
        for (auto it = this->begin(); it != this->end(); ++it) {
            if (m_compare(it->first, key)) {
                return it->second;
            }
        }

        throw(std::out_of_range("key not found"));
    }

    const T &at(Key &&key) const
    {
        for (auto it = this->begin(); it != this->end(); ++it) {
            if (m_compare(it->first, key)) {
                return it->second;
            }
        }

        throw(std::out_of_range("key not found"));
    }

    size_t erase(const key_type &key)
    {
        for (auto it = this->begin(); it != this->end(); ++it) {
            if (m_compare(it->first, key)) {
                for (auto next = it; ++next != this->end(); ++it) {
                    it->~value_type();
                    new (&*it) value_type{std::move(*next)};
                }
                Container::pop_back();
                return 1;
            }
        }
        return 0;
    }

    size_t erase(Key &&key)
    {
        for (auto it = this->begin(); it != this->end(); ++it) {
            if (m_compare(it->first, key)) {
                for (auto next = it; ++next != this->end(); ++it) {
                    it->~value_type();
                    new (&*it) value_type{std::move(*next)};
                }
                Container::pop_back();
                return 1;
            }
        }
        return 0;
    }

    iterator erase(iterator pos) { return erase(pos, std::next(pos)); }

    iterator erase(iterator first, iterator last)
    {
        if (first == last) {
            return first;
        }

        const auto elements_affected = std::distance(first, last);
        const auto offset = std::distance(Container::begin(), first);

        for (auto it = first; std::next(it, elements_affected) != Container::end(); ++it) {
            it->~value_type();
            new (&*it) value_type{std::move(*std::next(it, elements_affected))};
        }

        Container::resize(this->size() - static_cast<size_t>(elements_affected));

        return Container::begin() + offset;
    }

    size_t count(const key_type &key) const
    {
        for (auto it = this->begin(); it != this->end(); ++it) {
            if (m_compare(it->first, key)) {
                return 1;
            }
        }
        return 0;
    }

    size_t count(Key &&key) const
    {
        for (auto it = this->begin(); it != this->end(); ++it) {
            if (m_compare(it->first, key)) {
                return 1;
            }
        }
        return 0;
    }

    iterator find(const key_type &key)
    {
        for (auto it = this->begin(); it != this->end(); ++it) {
            if (m_compare(it->first, key)) {
                return it;
            }
        }
        return Container::end();
    }

    iterator find(Key &&key)
    {
        for (auto it = this->begin(); it != this->end(); ++it) {
            if (m_compare(it->first, key)) {
                return it;
            }
        }
        return Container::end();
    }

    const_iterator find(const key_type &key) const
    {
        for (auto it = this->begin(); it != this->end(); ++it) {
            if (m_compare(it->first, key)) {
                return it;
            }
        }
        return Container::end();
    }

    std::pair<iterator, bool> insert(value_type &&value)
    {
        return emplace(value.first, std::move(value.second));
    }

    std::pair<iterator, bool> insert(const value_type &value)
    {
        for (auto it = this->begin(); it != this->end(); ++it) {
            if (m_compare(it->first, value.first)) {
                return {it, false};
            }
        }
        Container::push_back(value);
        return {--this->end(), true};
    }

    template<typename InputIt>
    using require_input_iter = typename std::enable_if<
        std::is_convertible<typename std::iterator_traits<InputIt>::iterator_category,
                            std::input_iterator_tag>::value>::type;

    template<typename InputIt, typename = require_input_iter<InputIt>>
    void insert(InputIt first, InputIt last)
    {
        for (auto it = first; it != last; ++it) {
            insert(*it);
        }
    }

private:
    key_compare m_compare = key_compare();
};

} // namespace nlohmann
