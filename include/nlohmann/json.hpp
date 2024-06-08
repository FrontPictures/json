//     __ _____ _____ _____
//  __|  |   __|     |   | |  JSON for Modern C++
// |  |  |__   |  |  | | | |  version 3.11.3
// |_____|_____|_____|_|___|  https://github.com/nlohmann/json
//
// SPDX-FileCopyrightText: 2013-2023 Niels Lohmann <https://nlohmann.me>
// SPDX-License-Identifier: MIT

/****************************************************************************\
 * Note on documentation: The source files contain links to the online      *
 * documentation of the public API at https:
 * contains the most recent documentation and should also be applicable to  *
 * previous versions; documentation for deprecated functions is not         *
 * removed, but marked deprecated. See "Generate documentation" section in  *
 * file docs/README.md.                                                     *
\****************************************************************************/

#ifndef INCLUDE_NLOHMANN_JSON_HPP_
#define INCLUDE_NLOHMANN_JSON_HPP_

#include <cstddef>
#include <functional>
#include <initializer_list>
#include <iterator>
#include <string>
#include <utility>
#include <vector>

#include "nlohmann/byte_container_with_subtype.hpp"
#include "nlohmann/detail/exceptions.hpp"
#include "nlohmann/detail/input/input_format_t.h"
#include "nlohmann/detail/input/parser_types.h"
#include "nlohmann/detail/iterators/internal_iterator.hpp"
#include "nlohmann/detail/iterators/iter_impl.hpp"
#include "nlohmann/detail/iterators/iteration_proxy.hpp"
#include "nlohmann/detail/iterators/json_reverse_iterator.hpp"
#include "nlohmann/detail/iterators/primitive_iterator.hpp"
#include "nlohmann/detail/json_pointer.hpp"
#include "nlohmann/detail/json_ref.hpp"
#include "nlohmann/detail/value_t.hpp"
#include "nlohmann/ordered_map.hpp"
#include "nlohmann/detail/output/error_handler_t.h"

namespace nlohmann {
class json_sax;

namespace detail {
class json_sax_dom_callback_parser;
class json_sax_dom_parser;
class binary_reader;
class binary_writer;
class serializer;
class output_adapter;
class parser;
class iterator_input_adapter;
}

class ordered_json
{
private:
    friend class ::nlohmann::json_pointer;
    friend class ::nlohmann::detail::parser;
    friend class ::nlohmann::detail::serializer;
    friend class ::nlohmann::detail::iter_impl<ordered_json>;
    friend class ::nlohmann::detail::iter_impl<const ordered_json>;
    friend class ::nlohmann::detail::binary_writer;
    friend class ::nlohmann::detail::binary_reader;
    friend class ::nlohmann::detail::json_sax_dom_parser;
    friend class ::nlohmann::detail::json_sax_dom_callback_parser;
    friend class ::nlohmann::detail::exception;

    static ::nlohmann::detail::parser parser(const detail::iterator_input_adapter &adapter,
                                             detail::parser_callback_t cb = nullptr,
                                             const bool allow_exceptions = true,
                                             const bool ignore_comments = false);

    using primitive_iterator_t = ::nlohmann::detail::primitive_iterator_t;
    using internal_iterator = ::nlohmann::detail::internal_iterator<ordered_json>;
    template<typename T> using iter_impl = ::nlohmann::detail::iter_impl<T>;
    template<typename Iterator>
    using iteration_proxy = ::nlohmann::detail::iteration_proxy<Iterator>;
    template<typename Base>
    using json_reverse_iterator = ::nlohmann::detail::json_reverse_iterator<Base>;
    using binary_reader = ::nlohmann::detail::binary_reader;
    using binary_writer = ::nlohmann::detail::binary_writer;
    using serializer = ::nlohmann::detail::serializer;

public:
    using value_t = detail::value_t;
    using json_pointer = ::nlohmann::json_pointer;
    using error_handler_t = detail::error_handler_t;
    using cbor_tag_handler_t = detail::cbor_tag_handler_t;
    using initializer_list_t = std::initializer_list<detail::json_ref<ordered_json>>;
    using input_format_t = detail::input_format_t;
    using exception = detail::exception;
    using parse_error = detail::parse_error;
    using invalid_iterator = detail::invalid_iterator;
    using type_error = detail::type_error;
    using out_of_range = detail::out_of_range;
    using other_error = detail::other_error;
    using value_type = ordered_json;
    using reference = value_type &;
    using const_reference = const value_type &;
    using difference_type = std::ptrdiff_t;
    using pointer = ordered_json *;
    using const_pointer = const ordered_json *;
    using iterator = iter_impl<ordered_json>;
    using const_iterator = iter_impl<const ordered_json>;
    using reverse_iterator = json_reverse_iterator<typename ordered_json::iterator>;
    using const_reverse_iterator = json_reverse_iterator<typename ordered_json::const_iterator>;
    using object_t = nlohmann::ordered_map<std::string, ordered_json>;

    static ordered_json meta();

private:
    template<typename T, typename... Args> static T *create(Args &&...args)
    {
        return new T(std::forward<Args>(args)...);
    }

    union json_value {
        object_t *object;
        std::vector<ordered_json> *array;
        std::string *string;
        byte_container_with_subtype *binary;
        bool boolean;
        int64_t number_integer;
        double number_float;

        json_value() = default;
        json_value(bool v) noexcept : boolean(v) {}
        json_value(int64_t v) noexcept : number_integer(v) {}
        json_value(double v) noexcept : number_float(v) {}
        json_value(value_t t);
        json_value(const std::string &value) : string(create<std::string>(value)) {}
        json_value(std::string &&value) : string(create<std::string>(std::move(value))) {}
        json_value(const object_t &value) : object(create<object_t>(value)) {}
        json_value(object_t &&value) : object(create<object_t>(std::move(value))) {}

        json_value(const std::vector<ordered_json> &value)
            : array(create<std::vector<ordered_json>>(value))
        {}

        json_value(std::vector<ordered_json> &&value)
            : array(create<std::vector<ordered_json>>(std::move(value)))
        {}

        json_value(const std::vector<uint8_t> &value)
            : binary(create<byte_container_with_subtype>(value))
        {}

        json_value(std::vector<uint8_t> &&value)
            : binary(create<byte_container_with_subtype>(std::move(value)))
        {}

        json_value(const byte_container_with_subtype &value) : binary(create<byte_container_with_subtype>(value)) {}
        json_value(byte_container_with_subtype &&value) : binary(create<byte_container_with_subtype>(std::move(value))) {}
        void destroy(value_t t);
    };

public:
    using parse_event_t = detail::parse_event_t;
    using parser_callback_t = detail::parser_callback_t;

    ordered_json(const value_t v) : m_data(v) {}
    ordered_json(std::nullptr_t = nullptr) noexcept;
    ordered_json(bool b) noexcept;
    ordered_json(const char *s);
    ordered_json(std::string_view s);
    ordered_json(const std::string &s);
    ordered_json(std::string &&s);
    ordered_json(const byte_container_with_subtype &bin);
    ordered_json(byte_container_with_subtype &&bin);
    ordered_json(double val) noexcept;
    ordered_json(uint64_t val) noexcept;
    ordered_json(uint32_t val) noexcept;
    ordered_json(int64_t val) noexcept;
    ordered_json(int val) noexcept;
    ordered_json(const std::vector<ordered_json> &arr);
    ordered_json(std::vector<ordered_json> &&arr);
    ordered_json(const nlohmann::ordered_map<std::string, ordered_json> &obj);
    ordered_json(nlohmann::ordered_map<std::string, ordered_json> &&obj);
    ordered_json(initializer_list_t init, bool type_deduction = true,
                 value_t manual_type = value_t::array);

    template<typename Container, typename T = typename Container::value_type>
    ordered_json(const Container &values)
    {
        m_data.m_type = value_t::array;
        m_data.m_value.array = create<std::vector<ordered_json>>(values.begin(), values.end());
    }

    static ordered_json binary(const std::vector<uint8_t> &init);
    static ordered_json binary(const std::vector<uint8_t> &init,
                               typename byte_container_with_subtype::subtype_type subtype);
    static ordered_json binary(std::vector<uint8_t> &&init);
    static ordered_json binary(std::vector<uint8_t> &&init,
                               typename byte_container_with_subtype::subtype_type subtype);
    static ordered_json array(initializer_list_t init = {});
    static ordered_json object(initializer_list_t init = {});

    ordered_json(size_t cnt, const ordered_json &val) : m_data{cnt, val} {}
    ordered_json(const_iterator first, const_iterator last);
    ordered_json(const detail::json_ref<ordered_json> &r) : ordered_json(r.moved_or_copied()) {}
    ordered_json(const ordered_json &other);
    ordered_json(ordered_json &&other) noexcept;

    ordered_json &operator=(ordered_json other);
    ~ordered_json() noexcept {}

    std::string dump(const int indent = -1, const char indent_char = ' ',
                     const bool ensure_ascii = false,
                     const error_handler_t error_handler = error_handler_t::strict) const;

    value_t type() const noexcept { return m_data.m_type; }

    bool is_primitive() const noexcept;
    bool is_structured() const noexcept { return is_array() || is_object(); }
    bool is_null() const noexcept { return m_data.m_type == value_t::null; }
    bool is_boolean() const noexcept { return m_data.m_type == value_t::boolean; }
    bool is_number() const noexcept { return is_number_integer() || is_number_float(); }
    bool is_number_integer() const noexcept { return m_data.m_type == value_t::number_integer; }
    bool is_number_float() const noexcept { return m_data.m_type == value_t::number_float; }
    bool is_object() const noexcept { return m_data.m_type == value_t::object; }
    bool is_array() const noexcept { return m_data.m_type == value_t::array; }
    bool is_string() const noexcept { return m_data.m_type == value_t::string; }
    bool is_binary() const noexcept { return m_data.m_type == value_t::binary; }
    bool is_discarded() const noexcept { return m_data.m_type == value_t::discarded; }

    operator value_t() const noexcept { return m_data.m_type; }
    explicit operator bool() const { return get<bool>(); }
    explicit operator int() const { return get<int>(); }
    operator std::string() const { return get<std::string>(); }

private:
    object_t *get_impl_ptr(object_t *) noexcept;
    const object_t *get_impl_ptr(const object_t *) const noexcept;
    std::vector<ordered_json> *get_impl_ptr(std::vector<ordered_json> *) noexcept;
    const std::vector<ordered_json> *get_impl_ptr(const std::vector<ordered_json> *) const noexcept;
    std::string *get_impl_ptr(std::string *) noexcept;
    const std::string *get_impl_ptr(const std::string *) const noexcept;
    bool *get_impl_ptr(bool *) noexcept;
    const bool *get_impl_ptr(const bool *) const noexcept;
    int64_t *get_impl_ptr(uint32_t *) noexcept { return get_impl_ptr((int64_t *)nullptr); }
    const int64_t *get_impl_ptr(uint32_t *) const noexcept;
    int64_t *get_impl_ptr(int32_t *) noexcept { return get_impl_ptr((int64_t *)nullptr); }
    const int64_t *get_impl_ptr(int32_t *) const noexcept;
    int64_t *get_impl_ptr(int64_t *) noexcept;
    const int64_t *get_impl_ptr(const int64_t *) const noexcept;
    double *get_impl_ptr(double *) noexcept;
    const double *get_impl_ptr(const double *) const noexcept;
    byte_container_with_subtype *get_impl_ptr(byte_container_with_subtype *) noexcept;
    const byte_container_with_subtype *get_impl_ptr(const byte_container_with_subtype *) const noexcept;

public:
    const std::string &get_string_ref() const;

    byte_container_with_subtype &get_binary();
    const byte_container_with_subtype &get_binary() const;

    reference at(size_t idx);
    const_reference at(size_t idx) const;
    reference at(const std::string &key);
    const_reference at(const std::string &key) const;

    reference operator[](const size_t idx);
    reference operator[](int idx) { return (*this)[size_t(idx)]; }
    const_reference operator[](int idx) const { return (*this)[size_t(idx)]; }
    const_reference operator[](size_t idx) const;
    reference operator[](const char *cstr) { return (*this)[std::string(cstr)]; }
    reference operator[](std::string key);
    const_reference operator[](const std::string &key) const;

    template<typename T, std::enable_if_t<!std::is_enum<T>::value, int> = 0> T get() const
    {
        auto *ptr = get_impl_ptr(static_cast<T *>(nullptr));
        if (!ptr) {
            throw type_error::create(302, "invalid type");
        }
        return *ptr;
    }

    template<> int64_t get<int64_t>() const
    {
        if (is_boolean()) {
            return m_data.m_value.boolean ? 1 : 0;
        }
        if (is_number_integer()) {
            return m_data.m_value.number_integer;
        }
        if (is_number_float()) {
            return m_data.m_value.number_float;
        }
        throw type_error::create(302, "invalid type");
    }
    template<> int32_t get<int32_t>() const { return get<int64_t>(); }
    template<> uint32_t get<uint32_t>() const { return get<int64_t>(); }
    template<> uint64_t get<uint64_t>() const { return get<int64_t>(); }

    template<typename EnumType, std::enable_if_t<std::is_enum<EnumType>::value, int> = 0>
    EnumType get() const
    {
        using underlying_type = typename std::underlying_type<EnumType>::type;
        return EnumType(get<underlying_type>());
    }

    template<> double get<double>() const
    {
        if (is_number_integer()) {
            return m_data.m_value.number_integer;
        }
        if (is_number_float()) {
            return m_data.m_value.number_float;
        }
        throw type_error::create(302, "invalid type");
    }
    template<> float get<float>() const { return get<double>(); }

    template<typename Container, typename T = typename Container::value_type>
    Container getArray() const
    {
        if (!is_array()) {
            throw type_error::create(302, "invalid type");
        }
        Container result;
        result.reserve(m_data.m_value.array->size());
        for (const auto &item : *m_data.m_value.array) {
            result.push_back(item.get<T>());
        }
        return result;
    }

    template<> std::vector<int> get<std::vector<int>>() const
    {
        return getArray<std::vector<int>>();
    }
    template<> std::vector<float> get<std::vector<float>>() const
    {
        return getArray<std::vector<float>>();
    }
    template<> std::vector<std::string> get<std::vector<std::string>>() const
    {
        return getArray<std::vector<std::string>>();
    }

    template<typename ValueType>
    ValueType value(const std::string &key, const ValueType &default_value) const
    {
        if (is_object()) {
            const auto it = find(key);
            if (it != end()) {
                return it->get<ValueType>();
            }
            return default_value;
        }
        throw(type_error::create(306, std::string("cannot use value() with ") + type_name()));
    }

    template<class ValueType>
    ValueType value(const json_pointer &ptr, const ValueType &default_value) const
    {
        if (is_object()) {
            try {
                return ptr.get_checked(this).get<ValueType>();
            } catch (out_of_range &) {
                return default_value;
            }
        }
        throw(type_error::create(306, std::string("cannot use value() with ") + type_name()));
    }

    reference front() { return *begin(); }
    const_reference front() const { return *cbegin(); }
    reference back();
    const_reference back() const;

    iterator erase(iterator pos);
    iterator erase(iterator first, iterator last);

private:
    size_t erase_internal(const std::string &key);

public:
    size_t erase(const std::string &key) { return erase_internal(key); }
    void erase(const size_t idx);

    const_iterator find(const char *key) const { return find(std::string(key)); }
    const_iterator find(std::string_view key) const { return find(std::string(key)); }
    iterator find(const char *key) { return find(std::string(key)); }
    iterator find(std::string_view key) { return find(std::string(key)); }
    iterator find(const std::string &key);
    const_iterator find(const std::string &key) const;

    size_t count(const std::string &key) const;
    bool contains(const std::string &key) const;
    bool contains(const json_pointer &ptr) const { return ptr.contains(this); }

    iterator begin() noexcept;
    const_iterator begin() const noexcept { return cbegin(); }
    const_iterator cbegin() const noexcept;
    iterator end() noexcept;
    const_iterator end() const noexcept { return cend(); }
    const_iterator cend() const noexcept;
    reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
    const_reverse_iterator rbegin() const noexcept { return crbegin(); }
    reverse_iterator rend() noexcept { return reverse_iterator(begin()); }
    const_reverse_iterator rend() const noexcept { return crend(); }
    const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(cend()); }
    const_reverse_iterator crend() const noexcept { return const_reverse_iterator(cbegin()); }

public:
    iteration_proxy<iterator> items() noexcept { return iteration_proxy<iterator>(*this); }
    iteration_proxy<const_iterator> items() const noexcept
    {
        return iteration_proxy<const_iterator>(*this);
    }

    bool empty() const noexcept;
    size_t size() const noexcept;
    size_t max_size() const noexcept;
    void clear() noexcept;

    void push_back(ordered_json &&val);
    reference operator+=(ordered_json &&val);
    void push_back(const ordered_json &val);
    reference operator+=(const ordered_json &val);
    void push_back(const typename object_t::value_type &val);
    reference operator+=(const typename object_t::value_type &val);
    void push_back(initializer_list_t init);
    reference operator+=(initializer_list_t init);

    template<class... Args> reference emplace_back(Args &&...args)
    {
        if (!(is_null() || is_array())) {
            throw(type_error::create(311, std::string("cannot use emplace_back() with ") +
                                                         type_name()));
        }
        if (is_null()) {
            m_data.m_type = value_t::array;
            m_data.m_value = value_t::array;
        }
        m_data.m_value.array->emplace_back(std::forward<Args>(args)...);
        return m_data.m_value.array->back();
    }

    template<class... Args> std::pair<iterator, bool> emplace(Args &&...args)
    {
        if (!(is_null() || is_object())) {
            throw(type_error::create(311,
                                     std::string("cannot use emplace() with ") + type_name()));
        }
        if (is_null()) {
            m_data.m_type = value_t::object;
            m_data.m_value = value_t::object;
        }
        auto res = m_data.m_value.object->emplace(std::forward<Args>(args)...);
        auto it = begin();
        it.m_it.object_iterator = res.first;
        return {it, res.second};
    }

    template<typename... Args> iterator insert_iterator(const_iterator pos, Args &&...args)
    {
        iterator result(this);
        auto insert_pos = std::distance(m_data.m_value.array->begin(), pos.m_it.array_iterator);
        m_data.m_value.array->insert(pos.m_it.array_iterator, std::forward<Args>(args)...);
        result.m_it.array_iterator = m_data.m_value.array->begin() + insert_pos;
        return result;
    }

    iterator insert(const_iterator pos, const ordered_json &val);
    iterator insert(const_iterator pos, ordered_json &&val) { return insert(pos, val); }
    iterator insert(const_iterator pos, size_t cnt, const ordered_json &val);
    iterator insert(const_iterator pos, const_iterator first, const_iterator last);
    iterator insert(const_iterator pos, initializer_list_t ilist);
    void insert(const_iterator first, const_iterator last);

    void update(const_reference j, bool merge_objects = false);
    void update(const_iterator first, const_iterator last, bool merge_objects = false);

    void swap(reference other);
    friend void swap(reference left, reference right) { left.swap(right); }
    void swap(std::vector<ordered_json> &other);
    void swap(object_t &other);
    void swap(std::string &other);
    void swap(byte_container_with_subtype &other);
    void swap(std::vector<uint8_t> &other);

private:
    static bool compares_unordered(const_reference lhs, const_reference rhs,
                                   bool inverse = false) noexcept;

private:
    bool compares_unordered(const_reference rhs, bool inverse = false) const noexcept;

public:
    bool operator==(const_reference rhs) const noexcept;

    template<typename ScalarType>
        requires std::is_scalar_v<ScalarType> bool
    operator==(ScalarType rhs) const noexcept
    {
        return *this == ordered_json(rhs);
    }

    bool operator!=(const_reference rhs) const noexcept;

    std::partial_ordering operator<=>(const_reference rhs) const noexcept;

    template<typename ScalarType>
        requires std::is_scalar_v<ScalarType>
    std::partial_ordering operator<=>(ScalarType rhs) const noexcept
    {
        return *this <=> ordered_json(rhs);
    }

    static ordered_json parse(const char *str) { return parse(std::string(str)); }
    static ordered_json parse(std::string_view str) { return parse(std::string(str)); }
    static ordered_json parse(const std::string &str, const parser_callback_t cb = nullptr,
                              const bool allow_exceptions = true,
                              const bool ignore_comments = false);

    static bool accept(const std::string &i, const bool ignore_comments = false);

    static bool sax_parse(const std::string &i, json_sax *sax,
                          input_format_t format = input_format_t::json, const bool strict = true,
                          const bool ignore_comments = false);
    static bool sax_parse(const std::vector<uint8_t> &i, json_sax *sax,
                          input_format_t format = input_format_t::json, const bool strict = true,
                          const bool ignore_comments = false);

    const char *type_name() const noexcept;

private:
    class data
    {
    public:
        value_t m_type = value_t::null;
        json_value m_value = {};

        data(const value_t v) : m_type(v), m_value(v) {}
        data(size_t cnt, const ordered_json &val) : m_type(value_t::array)
        {
            m_value.array = create<std::vector<ordered_json>>(cnt, val);
        }

        data() noexcept = default;
        data(data &&) noexcept = default;
        data(const data &) noexcept = delete;
        data &operator=(data &&) noexcept = delete;
        data &operator=(const data &) noexcept = delete;
        ~data() noexcept { m_value.destroy(m_type); }
    };

    data m_data = {};

public:
    static std::vector<uint8_t> to_cbor(const ordered_json &j);
    static void to_cbor(const ordered_json &j, const detail::output_adapter &o);
    static std::vector<uint8_t> to_msgpack(const ordered_json &j);
    static void to_msgpack(const ordered_json &j, const detail::output_adapter &o);
    static std::vector<uint8_t> to_ubjson(const ordered_json &j, const bool use_size = false,
                                       const bool use_type = false);
    static void to_ubjson(const ordered_json &j, const detail::output_adapter &o,
                          const bool use_size = false, const bool use_type = false);
    static std::vector<uint8_t> to_bjdata(const ordered_json &j, const bool use_size = false,
                                       const bool use_type = false);
    static void to_bjdata(const ordered_json &j, const detail::output_adapter &o,
                          const bool use_size = false, const bool use_type = false);
    static std::vector<uint8_t> to_bson(const ordered_json &j);
    static void to_bson(const ordered_json &j, const detail::output_adapter &o);

    static ordered_json from_cbor(const std::vector<uint8_t> &i, const bool strict = true,
                                  const bool allow_exceptions = true,
                                  const cbor_tag_handler_t tag_handler = cbor_tag_handler_t::error);
    static ordered_json from_msgpack(const std::vector<uint8_t> &i, const bool strict = true,
                                     const bool allow_exceptions = true);
    static ordered_json from_ubjson(const std::vector<uint8_t> &i, const bool strict = true,
                                    const bool allow_exceptions = true);
    static ordered_json from_bjdata(const std::vector<uint8_t> &i, const bool strict = true,
                                    const bool allow_exceptions = true);
    static ordered_json from_bson(const std::vector<uint8_t> &i, const bool strict = true,
                                  const bool allow_exceptions = true);

    reference operator[](const json_pointer &ptr) { return ptr.get_unchecked(this); }
    const_reference operator[](const json_pointer &ptr) const { return ptr.get_unchecked(this); }

    reference at(const json_pointer &ptr) { return ptr.get_checked(this); }
    const_reference at(const json_pointer &ptr) const { return ptr.get_checked(this); }

    ordered_json flatten() const;
    ordered_json unflatten() const { return json_pointer::unflatten(*this); }

    void patch_inplace(const ordered_json &json_patch);
    ordered_json patch(const ordered_json &json_patch) const;

    static ordered_json diff(const ordered_json &source, const ordered_json &target,
                             const std::string &path = "");

    void merge_patch(const ordered_json &apply_patch);
};

using json = ordered_json;

inline std::string to_string(const ordered_json &j) { return j.dump(); }

inline namespace literals {
inline namespace json_literals {

inline nlohmann::json operator""_json(const char *s, std::size_t n)
{
    return nlohmann::json::parse(std::string(s, n));
}
inline nlohmann::json::json_pointer operator""_json_pointer(const char *s, std::size_t n)
{
    return nlohmann::json::json_pointer(std::string(s, n));
}

} // namespace json_literals
} // namespace literals

} // namespace nlohmann

namespace std {
template<> struct hash<nlohmann::ordered_json>
{
    size_t operator()(const nlohmann::ordered_json &j) const;
};
template<> struct less<::nlohmann::detail::value_t>
{
    bool operator()(::nlohmann::detail::value_t lhs, ::nlohmann::detail::value_t rhs) const noexcept
    {
        return std::is_lt(lhs <=> rhs);
    }
};
} // namespace std

#ifndef JSON_TEST_NO_GLOBAL_UDLS
using nlohmann::literals::json_literals::operator ""_json;
using nlohmann::literals::json_literals::operator ""_json_pointer;
#endif

#endif
