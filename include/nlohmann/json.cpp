#include "json.hpp"
#include <cmath>
#include "detail/input/json_sax.hpp"
#include "nlohmann/detail/string_escape.hpp"
#include "nlohmann/detail/hash.hpp"
#include "nlohmann/detail/string_concat.hpp"
#include "nlohmann/detail/input/input_adapters.hpp"
#include "nlohmann/detail/input/binary_reader.hpp"
#include "nlohmann/detail/output/binary_writer.hpp"
#include "nlohmann/detail/output/serializer.hpp"
#include "nlohmann/detail/output/output_adapters.hpp"
#include "nlohmann/detail/input/parser.hpp"

nlohmann::detail::parser nlohmann::ordered_json::parser(
    const detail::iterator_input_adapter &adapter, detail::parser_callback_t cb,
    const bool allow_exceptions, const bool ignore_comments)
{
    return ::nlohmann::detail::parser(adapter, std::move(cb), allow_exceptions, ignore_comments);
}

nlohmann::ordered_json nlohmann::ordered_json::meta()
{
    ordered_json result;

    result["copyright"] = "(C) 2013-2023 Niels Lohmann";
    result["name"] = "JSON for Modern C++ (non header only)";
    result["url"] = "https://github.com/nlohmann/json";
    result["version"]["string"] = "3.11.3";
    result["version"]["major"] = 3;
    result["version"]["minor"] = 11;
    result["version"]["patch"] = 3;

#ifdef _WIN32
    result["platform"] = "win32";
#elif defined __linux__
    result["platform"] = "linux";
#elif defined __APPLE__
    result["platform"] = "apple";
#elif defined __unix__
    result["platform"] = "unix";
#else
    result["platform"] = "unknown";
#endif

#if defined(__ICC) || defined(__INTEL_COMPILER)
    result["compiler"] = {{"family", "icc"}, {"version", __INTEL_COMPILER}};
#elif defined(__clang__)
    result["compiler"] = {{"family", "clang"}, {"version", __clang_version__}};
#elif defined(__GNUC__) || defined(__GNUG__)
    result["compiler"] = {{"family", "gcc"},
                          {"version", detail::concat(std::to_string(__GNUC__), '.',
                                                     std::to_string(__GNUC_MINOR__), '.',
                                                     std::to_string(__GNUC_PATCHLEVEL__))}};
#elif defined(__HP_cc) || defined(__HP_aCC)
    result["compiler"] = "hp"
#elif defined(__IBMCPP__)
    result["compiler"] = {{"family", "ilecpp"}, {"version", __IBMCPP__}};
#elif defined(_MSC_VER)
    result["compiler"] = {{"family", "msvc"}, {"version", _MSC_VER}};
#elif defined(__PGI)
    result["compiler"] = {{"family", "pgcpp"}, {"version", __PGI}};
#elif defined(__SUNPRO_CC)
    result["compiler"] = {{"family", "sunpro"}, {"version", __SUNPRO_CC}};
#else
    result["compiler"] = {{"family", "unknown"}, {"version", "unknown"}};
#endif

#if defined(_MSVC_LANG)
    result["compiler"]["c++"] = std::to_string(_MSVC_LANG);
#elif defined(__cplusplus)
    result["compiler"]["c++"] = std::to_string(__cplusplus);
#else
    result["compiler"]["c++"] = "unknown";
#endif
    return result;
}

nlohmann::ordered_json::ordered_json(std::nullptr_t) noexcept : ordered_json(value_t::null) {}

nlohmann::ordered_json::ordered_json(bool b) noexcept
{
    m_data.m_type = value_t::boolean;
    m_data.m_value = b;
}

nlohmann::ordered_json::ordered_json(const char *s)
{
    m_data.m_type = value_t::string;
    m_data.m_value = std::string(s);
}

nlohmann::ordered_json::ordered_json(std::string_view s)
{
    m_data.m_type = value_t::string;
    m_data.m_value = std::string(s);
}

nlohmann::ordered_json::ordered_json(const std::string &s)
{
    m_data.m_type = value_t::string;
    m_data.m_value = s;
}

nlohmann::ordered_json::ordered_json(std::string &&s)
{
    m_data.m_type = value_t::string;
    m_data.m_value = std::move(s);
}

nlohmann::ordered_json::ordered_json(const byte_container_with_subtype &bin)
{
    m_data.m_type = value_t::binary;
    m_data.m_value = nlohmann::byte_container_with_subtype(bin);
}

nlohmann::ordered_json::ordered_json(byte_container_with_subtype &&bin)
{
    m_data.m_type = value_t::binary;
    m_data.m_value = nlohmann::byte_container_with_subtype(std::move(bin));
}

nlohmann::ordered_json::ordered_json(double val) noexcept
{
    m_data.m_type = value_t::number_float;
    m_data.m_value = val;
}

nlohmann::ordered_json::ordered_json(uint64_t val) noexcept
{
    m_data.m_type = value_t::number_integer;
    m_data.m_value = int64_t(val);
}

nlohmann::ordered_json::ordered_json(uint32_t val) noexcept
{
    m_data.m_type = value_t::number_integer;
    m_data.m_value = int64_t(val);
}

nlohmann::ordered_json::ordered_json(int64_t val) noexcept
{
    m_data.m_type = value_t::number_integer;
    m_data.m_value = val;
}

nlohmann::ordered_json::ordered_json(int val) noexcept
{
    m_data.m_type = value_t::number_integer;
    m_data.m_value = int64_t(val);
}

nlohmann::ordered_json::ordered_json(const std::vector<ordered_json> &arr)
{
    m_data.m_type = value_t::array;
    m_data.m_value = arr;
}

nlohmann::ordered_json::ordered_json(std::vector<ordered_json> &&arr)
{
    m_data.m_type = value_t::array;
    m_data.m_value = std::move(arr);
}

nlohmann::ordered_json::ordered_json(const nlohmann::ordered_map<std::string, ordered_json> &obj)
{
    m_data.m_type = value_t::object;
    m_data.m_value = obj;
}

nlohmann::ordered_json::ordered_json(nlohmann::ordered_map<std::string, ordered_json> &&obj)
{
    m_data.m_type = value_t::object;
    m_data.m_value = std::move(obj);
}

nlohmann::ordered_json::ordered_json(initializer_list_t init, bool type_deduction,
                                     value_t manual_type)
{
    bool is_an_object = std::all_of(init.begin(), init.end(),
                                    [](const detail::json_ref<ordered_json> &element_ref) {
                                        return element_ref->is_array() && element_ref->size() == 2
                                               && (*element_ref)[static_cast<size_t>(0)].is_string();
                                    });

    if (!type_deduction) {
        if (manual_type == value_t::array) {
            is_an_object = false;
        }

        if (manual_type == value_t::object && !is_an_object) {
            throw(type_error::create(301, "cannot create object from initializer list"));
        }
    }

    if (is_an_object) {
        m_data.m_type = value_t::object;
        m_data.m_value = value_t::object;

        for (auto &element_ref : init) {
            auto element = element_ref.moved_or_copied();
            m_data.m_value.object
                ->emplace(std::move(*((*element.m_data.m_value.array)[0].m_data.m_value.string)),
                          std::move((*element.m_data.m_value.array)[1]));
        }
    } else {
        m_data.m_type = value_t::array;
        m_data.m_value.array = create<std::vector<ordered_json>>(init.begin(), init.end());
    }
}

nlohmann::ordered_json nlohmann::ordered_json::binary(const std::vector<uint8_t> &init)
{
    auto res = ordered_json();
    res.m_data.m_type = value_t::binary;
    res.m_data.m_value = init;
    return res;
}

nlohmann::ordered_json nlohmann::ordered_json::binary(
    const std::vector<uint8_t> &init, byte_container_with_subtype::subtype_type subtype)
{
    auto res = ordered_json();
    res.m_data.m_type = value_t::binary;
    res.m_data.m_value = byte_container_with_subtype(init, subtype);
    return res;
}

nlohmann::ordered_json nlohmann::ordered_json::binary(std::vector<uint8_t> &&init)
{
    auto res = ordered_json();
    res.m_data.m_type = value_t::binary;
    res.m_data.m_value = std::move(init);
    return res;
}

nlohmann::ordered_json nlohmann::ordered_json::binary(
    std::vector<uint8_t> &&init, byte_container_with_subtype::subtype_type subtype)
{
    auto res = ordered_json();
    res.m_data.m_type = value_t::binary;
    res.m_data.m_value = byte_container_with_subtype(std::move(init), subtype);
    return res;
}

nlohmann::ordered_json nlohmann::ordered_json::array(initializer_list_t init)
{
    return ordered_json(init, false, value_t::array);
}

nlohmann::ordered_json nlohmann::ordered_json::object(initializer_list_t init)
{
    return ordered_json(init, false, value_t::object);
}

nlohmann::ordered_json::ordered_json(const_iterator first, const_iterator last)
{
    if (first.m_object != last.m_object) {
        throw(invalid_iterator::create(201, "iterators are not compatible"));
    }

    m_data.m_type = first.m_object->m_data.m_type;

    switch (m_data.m_type) {
    case value_t::boolean:
    case value_t::number_float:
    case value_t::number_integer:
    case value_t::string: {
        if (!first.m_it.primitive_iterator.is_begin() || !last.m_it.primitive_iterator.is_end()) {
            throw(invalid_iterator::create(204, "iterators out of range"));
        }
        break;
    }

    case value_t::null:
    case value_t::object:
    case value_t::array:
    case value_t::binary:
    case value_t::discarded:
    default: break;
    }

    switch (m_data.m_type) {
    case value_t::number_integer: {
        m_data.m_value.number_integer = first.m_object->m_data.m_value.number_integer;
        break;
    }

    case value_t::number_float: {
        m_data.m_value.number_float = first.m_object->m_data.m_value.number_float;
        break;
    }

    case value_t::boolean: {
        m_data.m_value.boolean = first.m_object->m_data.m_value.boolean;
        break;
    }

    case value_t::string: {
        m_data.m_value = *first.m_object->m_data.m_value.string;
        break;
    }

    case value_t::object: {
        m_data.m_value.object = create<object_t>(first.m_it.object_iterator,
                                                 last.m_it.object_iterator);
        break;
    }

    case value_t::array: {
        m_data.m_value.array = create<std::vector<ordered_json>>(first.m_it.array_iterator,
                                                                 last.m_it.array_iterator);
        break;
    }

    case value_t::binary: {
        m_data.m_value = *first.m_object->m_data.m_value.binary;
        break;
    }

    case value_t::null:
    case value_t::discarded:
    default:
        throw(invalid_iterator::create(206, detail::concat("cannot construct with iterators from ",
                                                           first.m_object->type_name())));
    }
}

nlohmann::ordered_json::ordered_json(const ordered_json &other)
{
    m_data.m_type = other.m_data.m_type;

    switch (m_data.m_type) {
    case value_t::object: {
        m_data.m_value = *other.m_data.m_value.object;
        break;
    }

    case value_t::array: {
        m_data.m_value = *other.m_data.m_value.array;
        break;
    }

    case value_t::string: {
        m_data.m_value = *other.m_data.m_value.string;
        break;
    }

    case value_t::boolean: {
        m_data.m_value = other.m_data.m_value.boolean;
        break;
    }

    case value_t::number_integer: {
        m_data.m_value = other.m_data.m_value.number_integer;
        break;
    }

    case value_t::number_float: {
        m_data.m_value = other.m_data.m_value.number_float;
        break;
    }

    case value_t::binary: {
        m_data.m_value = *other.m_data.m_value.binary;
        break;
    }

    case value_t::null:
    case value_t::discarded:
    default: break;
    }
}

nlohmann::ordered_json::ordered_json(ordered_json &&other) noexcept
    : m_data(std::move(other.m_data))
{
    other.m_data.m_type = value_t::null;
    other.m_data.m_value = {};
}

nlohmann::ordered_json &nlohmann::ordered_json::operator=(ordered_json other)
{
    using std::swap;
    swap(m_data.m_type, other.m_data.m_type);
    swap(m_data.m_value, other.m_data.m_value);
    return *this;
}

std::string nlohmann::ordered_json::dump(const int indent, const char indent_char,
                                         const bool ensure_ascii,
                                         const error_handler_t error_handler) const
{
    std::string result;
    serializer s(detail::output_adapter(result), indent_char, error_handler);

    if (indent >= 0) {
        s.dump(*this, true, ensure_ascii, static_cast<unsigned int>(indent));
    } else {
        s.dump(*this, false, ensure_ascii, 0);
    }

    return result;
}

nlohmann::ordered_json::object_t *nlohmann::ordered_json::get_impl_ptr(object_t *) noexcept
{
    return is_object() ? m_data.m_value.object : nullptr;
}

const nlohmann::ordered_json::object_t *nlohmann::ordered_json::get_impl_ptr(
    const object_t *) const noexcept
{
    return is_object() ? m_data.m_value.object : nullptr;
}

std::vector<nlohmann::ordered_json> *nlohmann::ordered_json::get_impl_ptr(
    std::vector<ordered_json> *) noexcept
{
    return is_array() ? m_data.m_value.array : nullptr;
}

const std::vector<nlohmann::ordered_json> *nlohmann::ordered_json::get_impl_ptr(
    const std::vector<ordered_json> *) const noexcept
{
    return is_array() ? m_data.m_value.array : nullptr;
}

std::string *nlohmann::ordered_json::get_impl_ptr(std::string *) noexcept
{
    return is_string() ? m_data.m_value.string : nullptr;
}

const std::string *nlohmann::ordered_json::get_impl_ptr(const std::string *) const noexcept
{
    return is_string() ? m_data.m_value.string : nullptr;
}

bool *nlohmann::ordered_json::get_impl_ptr(bool *) noexcept
{
    return is_boolean() ? &m_data.m_value.boolean : nullptr;
}

const bool *nlohmann::ordered_json::get_impl_ptr(const bool *) const noexcept
{
    return is_boolean() ? &m_data.m_value.boolean : nullptr;
}

const int64_t *nlohmann::ordered_json::get_impl_ptr(uint32_t *) const noexcept
{
    return get_impl_ptr((int64_t *)nullptr);
}

const int64_t *nlohmann::ordered_json::get_impl_ptr(int32_t *) const noexcept
{
    return get_impl_ptr((int64_t *)nullptr);
}

int64_t *nlohmann::ordered_json::get_impl_ptr(int64_t *) noexcept
{
    return is_number_integer() ? &m_data.m_value.number_integer : nullptr;
}

const int64_t *nlohmann::ordered_json::get_impl_ptr(const int64_t *) const noexcept
{
    return is_number_integer() ? &m_data.m_value.number_integer : nullptr;
}

double *nlohmann::ordered_json::get_impl_ptr(double *) noexcept
{
    return is_number_float() ? &m_data.m_value.number_float : nullptr;
}

const double *nlohmann::ordered_json::get_impl_ptr(const double *) const noexcept
{
    return is_number_float() ? &m_data.m_value.number_float : nullptr;
}

nlohmann::byte_container_with_subtype *nlohmann::ordered_json::get_impl_ptr(
    byte_container_with_subtype *) noexcept
{
    return is_binary() ? m_data.m_value.binary : nullptr;
}

const nlohmann::byte_container_with_subtype *nlohmann::ordered_json::get_impl_ptr(
    const byte_container_with_subtype *) const noexcept
{
    return is_binary() ? m_data.m_value.binary : nullptr;
}

const std::string &nlohmann::ordered_json::get_string_ref() const
{
    auto *ptr = get_impl_ptr((const std::string *)nullptr);
    if (ptr != nullptr) {
        return *ptr;
    }
    throw(type_error::create(
        303,
        detail::concat("incompatible ReferenceType for get_ref, actual type is ", type_name())));
}

nlohmann::byte_container_with_subtype &nlohmann::ordered_json::get_binary()
{
    if (!is_binary()) {
        throw(type_error::create(302, detail::concat("type must be binary, but is ", type_name())));
    }
    return *get_impl_ptr((byte_container_with_subtype *)nullptr);
}

const nlohmann::byte_container_with_subtype &nlohmann::ordered_json::get_binary() const
{
    if (!is_binary()) {
        throw(type_error::create(302, detail::concat("type must be binary, but is ", type_name())));
    }

    return *get_impl_ptr((const byte_container_with_subtype *)nullptr);
}

nlohmann::ordered_json::reference nlohmann::ordered_json::at(size_t idx)
{
    if (is_array()) {
        try {
            return m_data.m_value.array->at(idx);
        } catch (std::out_of_range &) {
            throw(out_of_range::create(401, detail::concat("array index ", std::to_string(idx),
                                                           " is out of range")));
        }
    } else {
        throw(type_error::create(304, detail::concat("cannot use at() with ", type_name())));
    }
}

nlohmann::ordered_json::const_reference nlohmann::ordered_json::at(size_t idx) const
{
    if (is_array()) {
        try {
            return m_data.m_value.array->at(idx);
        } catch (std::out_of_range &) {
            throw(out_of_range::create(401, detail::concat("array index ", std::to_string(idx),
                                                           " is out of range")));
        }
    } else {
        throw(type_error::create(304, detail::concat("cannot use at() with ", type_name())));
    }
}

nlohmann::ordered_json::reference nlohmann::ordered_json::at(const std::string &key)
{
    if (!is_object()) {
        throw(type_error::create(304, detail::concat("cannot use at() with ", type_name())));
    }

    auto it = m_data.m_value.object->find(key);
    if (it == m_data.m_value.object->end()) {
        throw(out_of_range::create(403, detail::concat("key '", key, "' not found")));
    }
    return it->second;
}

nlohmann::ordered_json::const_reference nlohmann::ordered_json::at(const std::string &key) const
{
    if (!is_object()) {
        throw(type_error::create(304, detail::concat("cannot use at() with ", type_name())));
    }

    auto it = m_data.m_value.object->find(key);
    if (it == m_data.m_value.object->end()) {
        throw(out_of_range::create(403, detail::concat("key '", key, "' not found")));
    }
    return it->second;
}

nlohmann::ordered_json::reference nlohmann::ordered_json::back()
{
    auto tmp = end();
    --tmp;
    return *tmp;
}

nlohmann::ordered_json::const_reference nlohmann::ordered_json::back() const
{
    auto tmp = cend();
    --tmp;
    return *tmp;
}

nlohmann::ordered_json::iterator nlohmann::ordered_json::erase(iterator pos)
{
    if (this != pos.m_object) {
        throw(invalid_iterator::create(202, "iterator does not fit current value"));
    }

    iterator result = end();

    switch (m_data.m_type) {
    case value_t::boolean:
    case value_t::number_float:
    case value_t::number_integer:
    case value_t::string:
    case value_t::binary: {
        if (!pos.m_it.primitive_iterator.is_begin()) {
            throw(invalid_iterator::create(205, "iterator out of range"));
        }

        if (is_string()) {
            delete m_data.m_value.string;
            m_data.m_value.string = nullptr;
        } else if (is_binary()) {
            delete m_data.m_value.binary;
            m_data.m_value.binary = nullptr;
        }

        m_data.m_type = value_t::null;

        break;
    }

    case value_t::object: {
        result.m_it.object_iterator = m_data.m_value.object->erase(pos.m_it.object_iterator);
        break;
    }

    case value_t::array: {
        result.m_it.array_iterator = m_data.m_value.array->erase(pos.m_it.array_iterator);
        break;
    }

    case value_t::null:
    case value_t::discarded:
    default:
        throw(type_error::create(307, detail::concat("cannot use erase() with ", type_name())));
    }

    return result;
}

nlohmann::ordered_json::iterator nlohmann::ordered_json::erase(iterator first, iterator last)
{
    if (this != first.m_object || this != last.m_object) {
        throw(invalid_iterator::create(203, "iterators do not fit current value"));
    }

    iterator result = end();

    switch (m_data.m_type) {
    case value_t::boolean:
    case value_t::number_float:
    case value_t::number_integer:
    case value_t::string:
    case value_t::binary: {
        if (!first.m_it.primitive_iterator.is_begin() || !last.m_it.primitive_iterator.is_end()) {
            throw(invalid_iterator::create(204, "iterators out of range"));
        }

        if (is_string()) {
            delete m_data.m_value.string;
            m_data.m_value.string = nullptr;
        } else if (is_binary()) {
            delete m_data.m_value.binary;
            m_data.m_value.binary = nullptr;
        }

        m_data.m_type = value_t::null;

        break;
    }

    case value_t::object: {
        result.m_it.object_iterator = m_data.m_value.object->erase(first.m_it.object_iterator,
                                                                   last.m_it.object_iterator);
        break;
    }

    case value_t::array: {
        result.m_it.array_iterator = m_data.m_value.array->erase(first.m_it.array_iterator,
                                                                 last.m_it.array_iterator);
        break;
    }

    case value_t::null:
    case value_t::discarded:
    default:
        throw(type_error::create(307, detail::concat("cannot use erase() with ", type_name())));
    }

    return result;
}

size_t nlohmann::ordered_json::erase_internal(const std::string &key)
{
    if (!is_object()) {
        throw(type_error::create(307, detail::concat("cannot use erase() with ", type_name())));
    }

    const auto it = m_data.m_value.object->find(key);
    if (it != m_data.m_value.object->end()) {
        m_data.m_value.object->erase(it);
        return 1;
    }
    return 0;
}

void nlohmann::ordered_json::erase(const size_t idx)
{
    if (is_array()) {
        if (idx >= size()) {
            throw(out_of_range::create(401, detail::concat("array index ", std::to_string(idx),
                                                           " is out of range")));
        }

        m_data.m_value.array->erase(m_data.m_value.array->begin()
                                    + static_cast<difference_type>(idx));
    } else {
        throw(type_error::create(307, detail::concat("cannot use erase() with ", type_name())));
    }
}

nlohmann::ordered_json::iterator nlohmann::ordered_json::find(const std::string &key)
{
    auto result = end();

    if (is_object()) {
        result.m_it.object_iterator = m_data.m_value.object->find(key);
    }

    return result;
}

nlohmann::ordered_json::const_iterator nlohmann::ordered_json::find(const std::string &key) const
{
    auto result = cend();

    if (is_object()) {
        result.m_it.object_iterator = m_data.m_value.object->find(key);
    }

    return result;
}

size_t nlohmann::ordered_json::count(const std::string &key) const
{
    return is_object() ? m_data.m_value.object->count(key) : 0;
}

bool nlohmann::ordered_json::contains(const std::string &key) const
{
    return is_object() && m_data.m_value.object->find(key) != m_data.m_value.object->end();
}

nlohmann::ordered_json::iterator nlohmann::ordered_json::begin() noexcept
{
    iterator result(this);
    result.set_begin();
    return result;
}

nlohmann::ordered_json::const_iterator nlohmann::ordered_json::cbegin() const noexcept
{
    const_iterator result(this);
    result.set_begin();
    return result;
}

nlohmann::ordered_json::iterator nlohmann::ordered_json::end() noexcept
{
    iterator result(this);
    result.set_end();
    return result;
}

size_t nlohmann::ordered_json::size() const noexcept
{
    switch (m_data.m_type) {
    case value_t::null: {
        return 0;
    }

    case value_t::array: {
        return m_data.m_value.array->size();
    }

    case value_t::object: {
        return m_data.m_value.object->size();
    }

    case value_t::string:
    case value_t::boolean:
    case value_t::number_integer:
    case value_t::number_float:
    case value_t::binary:
    case value_t::discarded:
    default: {
        return 1;
    }
    }
}

void nlohmann::ordered_json::clear() noexcept
{
    switch (m_data.m_type) {
    case value_t::number_integer: {
        m_data.m_value.number_integer = 0;
        break;
    }

    case value_t::number_float: {
        m_data.m_value.number_float = 0.0;
        break;
    }

    case value_t::boolean: {
        m_data.m_value.boolean = false;
        break;
    }

    case value_t::string: {
        m_data.m_value.string->clear();
        break;
    }

    case value_t::binary: {
        m_data.m_value.binary->clear();
        break;
    }

    case value_t::array: {
        m_data.m_value.array->clear();
        break;
    }

    case value_t::object: {
        m_data.m_value.object->clear();
        break;
    }

    case value_t::null:
    case value_t::discarded:
    default: break;
    }
}

void nlohmann::ordered_json::push_back(ordered_json &&val)
{
    if (!(is_null() || is_array())) {
        throw(type_error::create(308, detail::concat("cannot use push_back() with ", type_name())));
    }

    if (is_null()) {
        m_data.m_type = value_t::array;
        m_data.m_value = value_t::array;
    }

    m_data.m_value.array->push_back(std::move(val));
}

void nlohmann::ordered_json::push_back(const ordered_json &val)
{
    if (!(is_null() || is_array())) {
        throw(type_error::create(308, detail::concat("cannot use push_back() with ", type_name())));
    }

    if (is_null()) {
        m_data.m_type = value_t::array;
        m_data.m_value = value_t::array;
    }

    m_data.m_value.array->push_back(val);
}

void nlohmann::ordered_json::push_back(const object_t::value_type &val)
{
    if (!(is_null() || is_object())) {
        throw(type_error::create(308, detail::concat("cannot use push_back() with ", type_name())));
    }

    if (is_null()) {
        m_data.m_type = value_t::object;
        m_data.m_value = value_t::object;
    }
    m_data.m_value.object->insert(val);
}

void nlohmann::ordered_json::push_back(initializer_list_t init)
{
    if (is_object() && init.size() == 2 && (*init.begin())->is_string()) {
        ordered_json &&key = init.begin()->moved_or_copied();
        push_back(typename object_t::value_type(std::move(*key.m_data.m_value.string),
                                                (init.begin() + 1)->moved_or_copied()));
    } else {
        push_back(ordered_json(init));
    }
}

nlohmann::ordered_json::reference nlohmann::ordered_json::operator+=(initializer_list_t init)
{
    push_back(init);
    return *this;
}

nlohmann::ordered_json::iterator nlohmann::ordered_json::insert(const_iterator pos,
                                                                const ordered_json &val)
{
    if (is_array()) {
        if (pos.m_object != this) {
            throw(invalid_iterator::create(202, "iterator does not fit current value"));
        }

        return insert_iterator(pos, val);
    }

    throw(type_error::create(309, detail::concat("cannot use insert() with ", type_name())));
}

nlohmann::ordered_json::iterator nlohmann::ordered_json::insert(const_iterator pos, size_t cnt,
                                                                const ordered_json &val)
{
    if (is_array()) {
        if (pos.m_object != this) {
            throw(invalid_iterator::create(202, "iterator does not fit current value"));
        }

        return insert_iterator(pos, cnt, val);
    }

    throw(type_error::create(309, detail::concat("cannot use insert() with ", type_name())));
}

nlohmann::ordered_json::iterator nlohmann::ordered_json::insert(const_iterator pos,
                                                                const_iterator first,
                                                                const_iterator last)
{
    if (!is_array()) {
        throw(type_error::create(309, detail::concat("cannot use insert() with ", type_name())));
    }

    if (pos.m_object != this) {
        throw(invalid_iterator::create(202, "iterator does not fit current value"));
    }

    if (first.m_object != last.m_object) {
        throw(invalid_iterator::create(210, "iterators do not fit"));
    }

    if (first.m_object == this) {
        throw(invalid_iterator::create(211, "passed iterators may not belong to container"));
    }

    return insert_iterator(pos, first.m_it.array_iterator, last.m_it.array_iterator);
}

nlohmann::ordered_json::iterator nlohmann::ordered_json::insert(const_iterator pos,
                                                                initializer_list_t ilist)
{
    if (!is_array()) {
        throw(type_error::create(309, detail::concat("cannot use insert() with ", type_name())));
    }

    if (pos.m_object != this) {
        throw(invalid_iterator::create(202, "iterator does not fit current value"));
    }

    return insert_iterator(pos, ilist.begin(), ilist.end());
}

void nlohmann::ordered_json::insert(const_iterator first, const_iterator last)
{
    if (!is_object()) {
        throw(type_error::create(309, detail::concat("cannot use insert() with ", type_name())));
    }

    if (first.m_object != last.m_object) {
        throw(invalid_iterator::create(210, "iterators do not fit"));
    }

    if (!first.m_object->is_object()) {
        throw(invalid_iterator::create(202, "iterators first and last must point to objects"));
    }

    m_data.m_value.object->insert(first.m_it.object_iterator, last.m_it.object_iterator);
}

void nlohmann::ordered_json::update(const_reference j, bool merge_objects)
{
    update(j.begin(), j.end(), merge_objects);
}

void nlohmann::ordered_json::update(const_iterator first, const_iterator last, bool merge_objects)
{
    if (is_null()) {
        m_data.m_type = value_t::object;
        m_data.m_value.object = create<object_t>();
    }

    if (!is_object()) {
        throw(type_error::create(312, detail::concat("cannot use update() with ", type_name())));
    }

    if (first.m_object != last.m_object) {
        throw(invalid_iterator::create(210, "iterators do not fit"));
    }

    if (!first.m_object->is_object()) {
        throw(type_error::create(312, detail::concat("cannot use update() with ",
                                                     first.m_object->type_name())));
    }

    for (auto it = first; it != last; ++it) {
        if (merge_objects && it.value().is_object()) {
            auto it2 = m_data.m_value.object->find(it.key());
            if (it2 != m_data.m_value.object->end()) {
                it2->second.update(it.value(), true);
                continue;
            }
        }
        m_data.m_value.object->operator[](it.key()) = it.value();
    }
}

void nlohmann::ordered_json::swap(reference other)
{
    std::swap(m_data.m_type, other.m_data.m_type);
    std::swap(m_data.m_value, other.m_data.m_value);
}

void nlohmann::ordered_json::swap(std::vector<ordered_json> &other)
{
    if (is_array()) {
        using std::swap;
        swap(*(m_data.m_value.array), other);
    } else {
        throw(type_error::create(310, detail::concat("cannot use swap(std::vector&) with ",
                                                     type_name())));
    }
}

void nlohmann::ordered_json::swap(object_t &other)
{
    if (is_object()) {
        using std::swap;
        swap(*(m_data.m_value.object), other);
    } else {
        throw(type_error::create(310,
                                 detail::concat("cannot use swap(object_t&) with ", type_name())));
    }
}

void nlohmann::ordered_json::swap(std::string &other)
{
    if (is_string()) {
        using std::swap;
        swap(*(m_data.m_value.string), other);
    } else {
        throw(type_error::create(310, detail::concat("cannot use swap(std::string&) with ",
                                                     type_name())));
    }
}

void nlohmann::ordered_json::swap(byte_container_with_subtype &other)
{
    if (is_binary()) {
        using std::swap;
        swap(*(m_data.m_value.binary), other);
    } else {
        throw(type_error::create(310,
                                 detail::concat("cannot use swap(binary_t&) with ", type_name())));
    }
}

void nlohmann::ordered_json::swap(std::vector<uint8_t> &other)
{
    if (is_binary()) {
        using std::swap;
        swap(*(m_data.m_value.binary), other);
    } else {
        throw(type_error::create(310,
                                 detail::concat("cannot use swap(binary_t::container_type&) with ",
                                                type_name())));
    }
}

bool nlohmann::ordered_json::compares_unordered(const_reference lhs, const_reference rhs,
                                                bool inverse) noexcept
{
    if ((lhs.is_number_float() && std::isnan(lhs.m_data.m_value.number_float) && rhs.is_number())
        || (rhs.is_number_float() && std::isnan(rhs.m_data.m_value.number_float)
            && lhs.is_number())) {
        return true;
    }
    static_cast<void>(inverse);
    return lhs.is_discarded() || rhs.is_discarded();
}

bool nlohmann::ordered_json::compares_unordered(const_reference rhs, bool inverse) const noexcept
{
    return compares_unordered(*this, rhs, inverse);
}

bool nlohmann::ordered_json::operator!=(const_reference rhs) const noexcept
{
    if (compares_unordered(rhs, true)) {
        return false;
    }
    return !operator==(rhs);
}

nlohmann::ordered_json nlohmann::ordered_json::parse(const std::string &str,
                                                     const parser_callback_t cb,
                                                     const bool allow_exceptions,
                                                     const bool ignore_comments)
{
    ordered_json result;
    parser(detail::input_adapter(str), cb, allow_exceptions, ignore_comments).parse(true, result);
    return result;
}

bool nlohmann::ordered_json::accept(const std::string &i, const bool ignore_comments)
{
    return parser(detail::input_adapter(i), nullptr, false, ignore_comments).accept(true);
}

bool nlohmann::ordered_json::sax_parse(const std::string &i, json_sax *sax, input_format_t format,
                                       const bool strict, const bool ignore_comments)
{
    auto ia = detail::input_adapter(i);
    return format == input_format_t::json
               ? parser(ia, nullptr, true, ignore_comments).sax_parse(sax, strict)
               : detail::binary_reader(ia, format).sax_parse(format, sax, strict);
}

bool nlohmann::ordered_json::sax_parse(const std::vector<uint8_t> &i, json_sax *sax,
                                       input_format_t format, const bool strict,
                                       const bool ignore_comments)
{
    auto ia = detail::input_adapter(i);
    return format == input_format_t::json
               ? parser(ia, nullptr, true, ignore_comments).sax_parse(sax, strict)
               : detail::binary_reader(ia, format).sax_parse(format, sax, strict);
}

const char *nlohmann::ordered_json::type_name() const noexcept
{
    switch (m_data.m_type) {
    case value_t::null: return "null";
    case value_t::object: return "object";
    case value_t::array: return "array";
    case value_t::string: return "string";
    case value_t::boolean: return "boolean";
    case value_t::binary: return "binary";
    case value_t::discarded: return "discarded";
    case value_t::number_integer:
    case value_t::number_float:
    default: return "number";
    }
}

std::vector<uint8_t> nlohmann::ordered_json::to_cbor(const ordered_json &j)
{
    std::vector<uint8_t> result;
    to_cbor(j, result);
    return result;
}

void nlohmann::ordered_json::to_cbor(const ordered_json &j, const detail::output_adapter &o)
{
    binary_writer(o).write_cbor(j);
}

std::vector<uint8_t> nlohmann::ordered_json::to_msgpack(const ordered_json &j)
{
    std::vector<uint8_t> result;
    to_msgpack(j, result);
    return result;
}

void nlohmann::ordered_json::to_msgpack(const ordered_json &j, const detail::output_adapter &o)
{
    binary_writer(o).write_msgpack(j);
}

std::vector<uint8_t> nlohmann::ordered_json::to_ubjson(const ordered_json &j, const bool use_size,
                                                    const bool use_type)
{
    std::vector<uint8_t> result;
    to_ubjson(j, result, use_size, use_type);
    return result;
}

void nlohmann::ordered_json::to_ubjson(const ordered_json &j, const detail::output_adapter &o,
                                       const bool use_size, const bool use_type)
{
    binary_writer(o).write_ubjson(j, use_size, use_type);
}

std::vector<uint8_t> nlohmann::ordered_json::to_bjdata(const ordered_json &j, const bool use_size,
                                                    const bool use_type)
{
    std::vector<uint8_t> result;
    to_bjdata(j, result, use_size, use_type);
    return result;
}

void nlohmann::ordered_json::to_bjdata(const ordered_json &j, const detail::output_adapter &o,
                                       const bool use_size, const bool use_type)
{
    binary_writer(o).write_ubjson(j, use_size, use_type, true, true);
}

std::vector<uint8_t> nlohmann::ordered_json::to_bson(const ordered_json &j)
{
    std::vector<uint8_t> result;
    to_bson(j, result);
    return result;
}

void nlohmann::ordered_json::to_bson(const ordered_json &j, const detail::output_adapter &o)
{
    binary_writer(o).write_bson(j);
}

#define JSON_IMPLEMENT_OPERATOR(op, null_result, unordered_result, default_result) \
    const auto lhs_type = lhs.type(); \
    const auto rhs_type = rhs.type(); \
    if (lhs_type == rhs_type) /* NOLINT(readability/braces) */ \
    { \
        switch (lhs_type) { \
        case value_t::array: return (*lhs.m_data.m_value.array)op(*rhs.m_data.m_value.array); \
        case value_t::object: return (*lhs.m_data.m_value.object)op(*rhs.m_data.m_value.object); \
        case value_t::null: return (null_result); \
        case value_t::string: return (*lhs.m_data.m_value.string)op(*rhs.m_data.m_value.string); \
        case value_t::boolean: return (lhs.m_data.m_value.boolean)op(rhs.m_data.m_value.boolean); \
        case value_t::number_integer: \
            return (lhs.m_data.m_value.number_integer)op(rhs.m_data.m_value.number_integer); \
        case value_t::number_float: \
            return (lhs.m_data.m_value.number_float)op(rhs.m_data.m_value.number_float); \
        case value_t::binary: return (*lhs.m_data.m_value.binary)op(*rhs.m_data.m_value.binary); \
        case value_t::discarded: \
        default: return (unordered_result); \
        } \
    } else if (lhs_type == value_t::number_integer && rhs_type == value_t::number_float) { \
        return static_cast<double>(lhs.m_data.m_value.number_integer) \
            op rhs.m_data.m_value.number_float; \
    } else if (lhs_type == value_t::number_float && rhs_type == value_t::number_integer) { \
        return lhs.m_data.m_value.number_float op static_cast<double>( \
            rhs.m_data.m_value.number_integer); \
    } else if (compares_unordered(lhs, rhs)) { \
        return (unordered_result); \
    } \
    return (default_result);

bool nlohmann::ordered_json::operator==(const_reference rhs) const noexcept
{
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
#endif
    const_reference lhs = *this;
    JSON_IMPLEMENT_OPERATOR(==, true, false, false)
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
}

std::partial_ordering nlohmann::ordered_json::operator<=>(const_reference rhs) const noexcept
{
    const_reference lhs = *this;

    JSON_IMPLEMENT_OPERATOR(<=>, std::partial_ordering::equivalent,
                            std::partial_ordering::unordered, lhs_type <=> rhs_type)
}

#undef JSON_IMPLEMENT_OPERATOR

nlohmann::ordered_json::reference nlohmann::ordered_json::operator+=(const object_t::value_type &val)
{
    push_back(val);
    return *this;
}

nlohmann::ordered_json::reference nlohmann::ordered_json::operator+=(const ordered_json &val)
{
    push_back(val);
    return *this;
}

nlohmann::ordered_json::reference nlohmann::ordered_json::operator+=(ordered_json &&val)
{
    push_back(std::move(val));
    return *this;
}

size_t nlohmann::ordered_json::max_size() const noexcept
{
    switch (m_data.m_type) {
    case value_t::array: {
        return m_data.m_value.array->max_size();
    }

    case value_t::object: {
        return m_data.m_value.object->max_size();
    }

    case value_t::null:
    case value_t::string:
    case value_t::boolean:
    case value_t::number_integer:
    case value_t::number_float:
    case value_t::binary:
    case value_t::discarded:
    default: {
        return size();
    }
    }
}

bool nlohmann::ordered_json::empty() const noexcept
{
    switch (m_data.m_type) {
    case value_t::null: {
        return true;
    }

    case value_t::array: {
        return m_data.m_value.array->empty();
    }

    case value_t::object: {
        return m_data.m_value.object->empty();
    }

    case value_t::string:
    case value_t::boolean:
    case value_t::number_integer:
    case value_t::number_float:
    case value_t::binary:
    case value_t::discarded:
    default: {
        return false;
    }
    }
}

nlohmann::ordered_json::const_iterator nlohmann::ordered_json::cend() const noexcept
{
    const_iterator result(this);
    result.set_end();
    return result;
}

nlohmann::ordered_json::const_reference nlohmann::ordered_json::operator[](
    const std::string &key) const
{
    if (is_object()) {
        auto it = m_data.m_value.object->find(key);

        return it->second;
    }

    throw(type_error::create(305,
                             detail::concat("cannot use operator[] with a string argument with ",
                                            type_name())));
}

nlohmann::ordered_json::reference nlohmann::ordered_json::operator[](std::string key)
{
    if (is_null()) {
        m_data.m_type = value_t::object;
        m_data.m_value.object = create<object_t>();
    }

    if (is_object()) {
        auto result = m_data.m_value.object->emplace(std::move(key), nullptr);
        return result.first->second;
    }

    throw(type_error::create(305,
                             detail::concat("cannot use operator[] with a string argument with ",
                                            type_name())));
}

nlohmann::ordered_json::const_reference nlohmann::ordered_json::operator[](size_t idx) const
{
    if (is_array()) {
        return m_data.m_value.array->operator[](idx);
    }

    throw(type_error::create(305,
                             detail::concat("cannot use operator[] with a numeric argument with ",
                                            type_name())));
}

nlohmann::ordered_json::reference nlohmann::ordered_json::operator[](const size_t idx)
{
    if (is_null()) {
        m_data.m_type = value_t::array;
        m_data.m_value.array = create<std::vector<ordered_json>>();
    }

    if (is_array()) {
        if (idx >= m_data.m_value.array->size()) {
            m_data.m_value.array->resize(idx + 1);
        }

        return m_data.m_value.array->operator[](idx);
    }

    throw(type_error::create(305,
                             detail::concat("cannot use operator[] with a numeric argument with ",
                                            type_name())));
}

bool nlohmann::ordered_json::is_primitive() const noexcept
{
    return is_null() || is_string() || is_boolean() || is_number() || is_binary();
}

nlohmann::ordered_json nlohmann::ordered_json::from_cbor(const std::vector<uint8_t> &i,
                                                         const bool strict,
                                                         const bool allow_exceptions,
                                                         const cbor_tag_handler_t tag_handler)
{
    ordered_json result;
    detail::json_sax_dom_parser sdp(result, allow_exceptions);
    auto ia = detail::input_adapter(i);
    const bool res = binary_reader(ia, input_format_t::cbor)
                         .sax_parse(input_format_t::cbor, &sdp, strict, tag_handler);
    return res ? result : ordered_json(value_t::discarded);
}

nlohmann::ordered_json nlohmann::ordered_json::from_msgpack(const std::vector<uint8_t> &i,
                                                            const bool strict,
                                                            const bool allow_exceptions)
{
    ordered_json result;
    detail::json_sax_dom_parser sdp(result, allow_exceptions);
    auto ia = detail::input_adapter(i);
    const bool res = binary_reader(ia, input_format_t::msgpack)
                         .sax_parse(input_format_t::msgpack, &sdp, strict);
    return res ? result : ordered_json(value_t::discarded);
}

nlohmann::ordered_json nlohmann::ordered_json::from_ubjson(const std::vector<uint8_t> &i,
                                                           const bool strict,
                                                           const bool allow_exceptions)
{
    ordered_json result;
    detail::json_sax_dom_parser sdp(result, allow_exceptions);
    auto ia = detail::input_adapter(i);
    const bool res = binary_reader(ia, input_format_t::ubjson)
                         .sax_parse(input_format_t::ubjson, &sdp, strict);
    return res ? result : ordered_json(value_t::discarded);
}

nlohmann::ordered_json nlohmann::ordered_json::from_bjdata(const std::vector<uint8_t> &i,
                                                           const bool strict,
                                                           const bool allow_exceptions)
{
    ordered_json result;
    detail::json_sax_dom_parser sdp(result, allow_exceptions);
    auto ia = detail::input_adapter(i);
    const bool res = binary_reader(ia, input_format_t::bjdata)
                         .sax_parse(input_format_t::bjdata, &sdp, strict);
    return res ? result : ordered_json(value_t::discarded);
}

nlohmann::ordered_json nlohmann::ordered_json::from_bson(const std::vector<uint8_t> &i,
                                                         const bool strict,
                                                         const bool allow_exceptions)
{
    ordered_json result;
    detail::json_sax_dom_parser sdp(result, allow_exceptions);
    auto ia = detail::input_adapter(i);
    const bool res = binary_reader(ia, input_format_t::bson)
                         .sax_parse(input_format_t::bson, &sdp, strict);
    return res ? result : ordered_json(value_t::discarded);
}

nlohmann::ordered_json nlohmann::ordered_json::flatten() const
{
    ordered_json result(value_t::object);
    json_pointer::flatten("", *this, result);
    return result;
}

void nlohmann::ordered_json::patch_inplace(const ordered_json &json_patch)
{
    ordered_json &result = *this;

    enum class patch_operations { add, remove, replace, move, copy, test, invalid };

    const auto get_op = [](const std::string &op) {
        if (op == "add") {
            return patch_operations::add;
        }
        if (op == "remove") {
            return patch_operations::remove;
        }
        if (op == "replace") {
            return patch_operations::replace;
        }
        if (op == "move") {
            return patch_operations::move;
        }
        if (op == "copy") {
            return patch_operations::copy;
        }
        if (op == "test") {
            return patch_operations::test;
        }

        return patch_operations::invalid;
    };

    const auto operation_add = [&result](json_pointer &ptr, ordered_json val) {
        if (ptr.empty()) {
            result = val;
            return;
        }

        json_pointer const top_pointer = ptr.top();
        if (top_pointer != ptr) {
            result.at(top_pointer);
        }

        const auto last_path = ptr.back();
        ptr.pop_back();

        ordered_json &parent = result.at(ptr);

        switch (parent.m_data.m_type) {
        case value_t::null:
        case value_t::object: {
            parent[last_path] = val;
            break;
        }

        case value_t::array: {
            if (last_path == "-") {
                parent.push_back(val);
            } else {
                const auto idx = json_pointer::array_index(last_path);
                if (idx > parent.size()) {
                    throw(out_of_range::create(401,
                                               detail::concat("array index ", std::to_string(idx),
                                                              " is out of range")));
                }

                parent.insert(parent.begin() + static_cast<difference_type>(idx), val);
            }
            break;
        }

        case value_t::string:
        case value_t::boolean:
        case value_t::number_integer:
        case value_t::number_float:
        case value_t::binary:
        case value_t::discarded:
        default: break;
        }
    };

    const auto operation_remove = [&result](json_pointer &ptr) {
        const auto last_path = ptr.back();
        ptr.pop_back();
        ordered_json &parent = result.at(ptr);

        if (parent.is_object()) {
            auto it = parent.find(last_path);
            if (it != parent.end()) {
                parent.erase(it);
            } else {
                throw(out_of_range::create(403, detail::concat("key '", last_path, "' not found")));
            }
        } else if (parent.is_array()) {
            parent.erase(json_pointer::array_index(last_path));
        }
    };

    if (!json_patch.is_array()) {
        throw(parse_error::create(104, 0, "JSON patch must be an array of objects"));
    }

    for (const auto &val : json_patch) {
        const auto get_value = [&val](const std::string &op, const std::string &member,
                                      bool string_type) -> ordered_json & {
            auto it = val.m_data.m_value.object->find(member);

            const auto error_msg = (op == "op") ? "operation"
                                                : detail::concat("operation '", op, '\'');

            if (it == val.m_data.m_value.object->end()) {
                throw(parse_error::create(105, 0,
                                          detail::concat(error_msg, " must have member '", member,
                                                         "'")));
            }

            if (string_type && !it->second.is_string()) {
                throw(parse_error::create(105, 0,
                                          detail::concat(error_msg, " must have string member '",
                                                         member, "'")));
            }

            return it->second;
        };

        if (!val.is_object()) {
            throw(parse_error::create(104, 0, "JSON patch must be an array of objects"));
        }

        const auto op = get_value("op", "op", true).get<std::string>();
        const auto path = get_value(op, "path", true).get<std::string>();
        json_pointer ptr(path);

        switch (get_op(op)) {
        case patch_operations::add: {
            operation_add(ptr, get_value("add", "value", false));
            break;
        }

        case patch_operations::remove: {
            operation_remove(ptr);
            break;
        }

        case patch_operations::replace: {
            result.at(ptr) = get_value("replace", "value", false);
            break;
        }

        case patch_operations::move: {
            const auto from_path = get_value("move", "from", true).get<std::string>();
            json_pointer from_ptr(from_path);

            ordered_json const v = result.at(from_ptr);

            operation_remove(from_ptr);
            operation_add(ptr, v);
            break;
        }

        case patch_operations::copy: {
            const auto from_path = get_value("copy", "from", true).get<std::string>();
            const json_pointer from_ptr(from_path);

            ordered_json const v = result.at(from_ptr);

            operation_add(ptr, v);
            break;
        }

        case patch_operations::test: {
            bool success = false;
            try {
                success = (result.at(ptr) == get_value("test", "value", false));
            } catch (out_of_range &) {}

            if (!success) {
                throw(other_error::create(501, detail::concat("unsuccessful: ", val.dump())));
            }

            break;
        }

        case patch_operations::invalid:
        default: {
            throw(parse_error::create(105, 0,
                                      detail::concat("operation value '", op, "' is invalid")));
        }
        }
    }
}

nlohmann::ordered_json nlohmann::ordered_json::patch(const ordered_json &json_patch) const
{
    ordered_json result = *this;
    result.patch_inplace(json_patch);
    return result;
}

nlohmann::ordered_json nlohmann::ordered_json::diff(const ordered_json &source,
                                                    const ordered_json &target,
                                                    const std::string &path)
{
    ordered_json result(value_t::array);

    if (source == target) {
        return result;
    }

    if (source.type() != target.type()) {
        result.push_back({{"op", "replace"}, {"path", path}, {"value", target}});
        return result;
    }

    switch (source.type()) {
    case value_t::array: {
        size_t i = 0;
        while (i < source.size() && i < target.size()) {
            auto temp_diff = diff(source[i], target[i],
                                  detail::concat(path, '/', std::to_string(i)));
            result.insert(result.end(), temp_diff.begin(), temp_diff.end());
            ++i;
        }

        const auto end_index = static_cast<difference_type>(result.size());
        while (i < source.size()) {
            result.insert(result.begin() + end_index,
                          object({{"op", "remove"},
                                  {"path", detail::concat(path, '/', std::to_string(i))}}));
            ++i;
        }

        while (i < target.size()) {
            result.push_back(
                {{"op", "add"}, {"path", detail::concat(path, "/-")}, {"value", target[i]}});
            ++i;
        }

        break;
    }

    case value_t::object: {
        for (auto it = source.cbegin(); it != source.cend(); ++it) {
            const auto path_key = detail::concat(path, '/', detail::escape(it.key()));

            if (target.find(it.key()) != target.end()) {
                auto temp_diff = diff(it.value(), target[it.key()], path_key);
                result.insert(result.end(), temp_diff.begin(), temp_diff.end());
            } else {
                result.push_back(object({{"op", "remove"}, {"path", path_key}}));
            }
        }

        for (auto it = target.cbegin(); it != target.cend(); ++it) {
            if (source.find(it.key()) == source.end()) {
                const auto path_key = detail::concat(path, '/', detail::escape(it.key()));
                result.push_back({{"op", "add"}, {"path", path_key}, {"value", it.value()}});
            }
        }

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
        result.push_back({{"op", "replace"}, {"path", path}, {"value", target}});
        break;
    }
    }

    return result;
}

void nlohmann::ordered_json::merge_patch(const ordered_json &apply_patch)
{
    if (apply_patch.is_object()) {
        if (!is_object()) {
            *this = object();
        }
        for (auto it = apply_patch.begin(); it != apply_patch.end(); ++it) {
            if (it.value().is_null()) {
                erase(it.key());
            } else {
                operator[](it.key()).merge_patch(it.value());
            }
        }
    } else {
        *this = apply_patch;
    }
}

nlohmann::ordered_json::json_value::json_value(value_t t)
{
    switch (t) {
    case value_t::object: {
        object = create<object_t>();
        break;
    }

    case value_t::array: {
        array = create<std::vector<ordered_json>>();
        break;
    }

    case value_t::string: {
        string = create<std::string>("");
        break;
    }

    case value_t::binary: {
        binary = create<byte_container_with_subtype>();
        break;
    }

    case value_t::boolean: {
        boolean = static_cast<bool>(false);
        break;
    }

    case value_t::number_integer: {
        number_integer = static_cast<int64_t>(0);
        break;
    }

    case value_t::number_float: {
        number_float = static_cast<double>(0.0);
        break;
    }

    case value_t::null: {
        object = nullptr;
        break;
    }

    case value_t::discarded:
    default: {
        object = nullptr;
        if (t == value_t::null) {
            throw(other_error::create(500, "961c151d2e87f2686a955a9be24d316f1362bf21 3.11.3"));
        }
        break;
    }
    }
}

void nlohmann::ordered_json::json_value::destroy(value_t t)
{
    if ((t == value_t::object && object == nullptr) || (t == value_t::array && array == nullptr)
        || (t == value_t::string && string == nullptr)
        || (t == value_t::binary && binary == nullptr)) {
        return;
    }
    if (t == value_t::array || t == value_t::object) {
        std::vector<ordered_json> stack;

        if (t == value_t::array) {
            stack.reserve(array->size());
            std::move(array->begin(), array->end(), std::back_inserter(stack));
        } else {
            stack.reserve(object->size());
            for (auto &&it : *object) {
                stack.push_back(std::move(it.second));
            }
        }

        while (!stack.empty()) {
            ordered_json current_item(std::move(stack.back()));
            stack.pop_back();

            if (current_item.is_array()) {
                std::move(current_item.m_data.m_value.array->begin(),
                          current_item.m_data.m_value.array->end(), std::back_inserter(stack));

                current_item.m_data.m_value.array->clear();
            } else if (current_item.is_object()) {
                for (auto &&it : *current_item.m_data.m_value.object) {
                    stack.push_back(std::move(it.second));
                }

                current_item.m_data.m_value.object->clear();
            }
        }
    }

    switch (t) {
    case value_t::object: {
        delete object;
        break;
    }

    case value_t::array: {
        delete array;
        break;
    }

    case value_t::string: {
        delete string;
        break;
    }

    case value_t::binary: {
        delete binary;
        break;
    }

    case value_t::null:
    case value_t::boolean:
    case value_t::number_integer:
    case value_t::number_float:
    case value_t::discarded:
    default: {
        break;
    }
    }
}

std::size_t std::hash<nlohmann::ordered_json>::operator()(const nlohmann::ordered_json &j) const
{
    return nlohmann::detail::hash(j);
}
