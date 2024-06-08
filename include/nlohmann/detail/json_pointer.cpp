#include "json_pointer.hpp"
#include <algorithm>
#include <cctype>
#include <cerrno>
#include <limits>
#include <numeric>
#include "nlohmann/detail/exceptions.hpp"
#include "nlohmann/detail/string_escape.hpp"
#include "string_concat.hpp"
#include "nlohmann/json.hpp"

std::string nlohmann::json_pointer::to_string() const
{
    return std::accumulate(reference_tokens.begin(), reference_tokens.end(), std::string{},
                           [](const std::string &a, const std::string &b) {
                               return detail::concat(a, '/', detail::escape(b));
                           });
}

nlohmann::json_pointer nlohmann::json_pointer::parent_pointer() const
{
    if (empty()) {
        return *this;
    }

    json_pointer res = *this;
    res.pop_back();
    return res;
}

void nlohmann::json_pointer::pop_back()
{
    if (empty()) {
        throw(detail::out_of_range::create(405, "JSON pointer has no parent"));
    }

    reference_tokens.pop_back();
}

const std::string &nlohmann::json_pointer::back() const
{
    if (empty()) {
        throw(detail::out_of_range::create(405, "JSON pointer has no parent"));
    }

    return reference_tokens.back();
}

size_t nlohmann::json_pointer::array_index(const std::string &s)
{
    using size_t = size_t;

    if (s.size() > 1 && s[0] == '0') {
        throw(detail::parse_error::create(106, 0,
                                          detail::concat("array index '", s,
                                                         "' must not begin with '0'")));
    }

    if (s.size() > 1 && !(s[0] >= '1' && s[0] <= '9')) {
        throw(detail::parse_error::create(109, 0,
                                          detail::concat("array index '", s, "' is not a number")));
    }

    const char *p = s.c_str();
    char *p_end = nullptr;
    errno = 0;
    const unsigned long long res = std::strtoull(p, &p_end, 10);
    if (p == p_end || errno == ERANGE || static_cast<size_t>(p_end - p) != s.size()) {
        throw(detail::out_of_range::create(404, detail::concat("unresolved reference token '", s,
                                                               "'")));
    }

    if (res >= static_cast<unsigned long long>((std::numeric_limits<size_t>::max)())) {
        throw(detail::out_of_range::create(410,
                                           detail::concat("array index ", s, " exceeds size_t")));
    }

    return static_cast<size_t>(res);
}

nlohmann::json_pointer nlohmann::json_pointer::top() const
{
    if (empty()) {
        throw(detail::out_of_range::create(405, "JSON pointer has no parent"));
    }

    json_pointer result = *this;
    result.reference_tokens = {reference_tokens[0]};
    return result;
}

nlohmann::json_pointer &nlohmann::json_pointer::operator/=(std::string token)
{
    push_back(std::move(token));
    return *this;
}

nlohmann::json_pointer &nlohmann::json_pointer::operator/=(const json_pointer &ptr)
{
    reference_tokens.insert(reference_tokens.end(), ptr.reference_tokens.begin(),
                            ptr.reference_tokens.end());
    return *this;
}

nlohmann::ordered_json &nlohmann::json_pointer::get_and_create(ordered_json &j) const
{
    auto *result = &j;

    for (const auto &reference_token : reference_tokens) {
        switch (result->type()) {
        case detail::value_t::null: {
            if (reference_token == "0") {
                result = &result->operator[](0);
            } else {
                result = &result->operator[](reference_token);
            }
            break;
        }

        case detail::value_t::object: {
            result = &result->operator[](reference_token);
            break;
        }

        case detail::value_t::array: {
            result = &result->operator[](array_index(reference_token));
            break;
        }

        case detail::value_t::string:
        case detail::value_t::boolean:
        case detail::value_t::number_integer:
        case detail::value_t::number_float:
        case detail::value_t::binary:
        case detail::value_t::discarded:
        default: throw(detail::type_error::create(313, "invalid value to unflatten"));
        }
    }

    return *result;
}

nlohmann::ordered_json &nlohmann::json_pointer::get_unchecked(ordered_json *ptr) const
{
    for (const auto &reference_token : reference_tokens) {
        if (ptr->is_null()) {
            const bool nums = std::all_of(reference_token.begin(), reference_token.end(),
                                          [](const unsigned char x) { return std::isdigit(x); });

            *ptr = (nums || reference_token == "-") ? detail::value_t::array
                                                    : detail::value_t::object;
        }

        switch (ptr->type()) {
        case detail::value_t::object: {
            ptr = &ptr->operator[](reference_token);
            break;
        }

        case detail::value_t::array: {
            if (reference_token == "-") {
                ptr = &ptr->operator[](ptr->m_data.m_value.array->size());
            } else {
                ptr = &ptr->operator[](array_index(reference_token));
            }
            break;
        }

        case detail::value_t::null:
        case detail::value_t::string:
        case detail::value_t::boolean:
        case detail::value_t::number_integer:
        case detail::value_t::number_float:
        case detail::value_t::binary:
        case detail::value_t::discarded:
        default:
            throw(detail::out_of_range::create(404, detail::concat("unresolved reference token '",
                                                                   reference_token, "'")));
        }
    }

    return *ptr;
}

nlohmann::ordered_json &nlohmann::json_pointer::get_checked(ordered_json *ptr) const
{
    for (const auto &reference_token : reference_tokens) {
        switch (ptr->type()) {
        case detail::value_t::object: {
            ptr = &ptr->at(reference_token);
            break;
        }

        case detail::value_t::array: {
            if (reference_token == "-") {
                throw(detail::out_of_range::create(
                    402, detail::concat("array index '-' (",
                                        std::to_string(ptr->m_data.m_value.array->size()),
                                        ") is out of range")));
            }

            ptr = &ptr->at(array_index(reference_token));
            break;
        }

        case detail::value_t::null:
        case detail::value_t::string:
        case detail::value_t::boolean:
        case detail::value_t::number_integer:
        case detail::value_t::number_float:
        case detail::value_t::binary:
        case detail::value_t::discarded:
        default:
            throw(detail::out_of_range::create(404, detail::concat("unresolved reference token '",
                                                                   reference_token, "'")));
        }
    }

    return *ptr;
}

const nlohmann::ordered_json &nlohmann::json_pointer::get_unchecked(const ordered_json *ptr) const
{
    for (const auto &reference_token : reference_tokens) {
        switch (ptr->type()) {
        case detail::value_t::object: {
            ptr = &ptr->operator[](reference_token);
            break;
        }

        case detail::value_t::array: {
            if (reference_token == "-") {
                throw(detail::out_of_range::create(
                    402, detail::concat("array index '-' (",
                                        std::to_string(ptr->m_data.m_value.array->size()),
                                        ") is out of range")));
            }

            ptr = &ptr->operator[](array_index(reference_token));
            break;
        }

        case detail::value_t::null:
        case detail::value_t::string:
        case detail::value_t::boolean:
        case detail::value_t::number_integer:
        case detail::value_t::number_float:
        case detail::value_t::binary:
        case detail::value_t::discarded:
        default:
            throw(detail::out_of_range::create(404, detail::concat("unresolved reference token '",
                                                                   reference_token, "'")));
        }
    }

    return *ptr;
}

const nlohmann::ordered_json &nlohmann::json_pointer::get_checked(const ordered_json *ptr) const
{
    for (const auto &reference_token : reference_tokens) {
        switch (ptr->type()) {
        case detail::value_t::object: {
            ptr = &ptr->at(reference_token);
            break;
        }

        case detail::value_t::array: {
            if (reference_token == "-") {
                throw(detail::out_of_range::create(
                    402, detail::concat("array index '-' (",
                                        std::to_string(ptr->m_data.m_value.array->size()),
                                        ") is out of range")));
            }

            ptr = &ptr->at(array_index(reference_token));
            break;
        }

        case detail::value_t::null:
        case detail::value_t::string:
        case detail::value_t::boolean:
        case detail::value_t::number_integer:
        case detail::value_t::number_float:
        case detail::value_t::binary:
        case detail::value_t::discarded:
        default:
            throw(detail::out_of_range::create(404, detail::concat("unresolved reference token '",
                                                                   reference_token, "'")));
        }
    }

    return *ptr;
}

bool nlohmann::json_pointer::contains(const ordered_json *ptr) const
{
    for (const auto &reference_token : reference_tokens) {
        switch (ptr->type()) {
        case detail::value_t::object: {
            if (!ptr->contains(reference_token)) {
                return false;
            }

            ptr = &ptr->operator[](reference_token);
            break;
        }

        case detail::value_t::array: {
            if (reference_token == "-") {
                return false;
            }
            if (reference_token.size() == 1
                && !("0" <= reference_token && reference_token <= "9")) {
                return false;
            }
            if (reference_token.size() > 1) {
                if (!('1' <= reference_token[0] && reference_token[0] <= '9')) {
                    return false;
                }
                for (size_t i = 1; i < reference_token.size(); i++) {
                    if (!('0' <= reference_token[i] && reference_token[i] <= '9')) {
                        return false;
                    }
                }
            }

            const auto idx = array_index(reference_token);
            if (idx >= ptr->size()) {
                return false;
            }

            ptr = &ptr->operator[](idx);
            break;
        }

        case detail::value_t::null:
        case detail::value_t::string:
        case detail::value_t::boolean:
        case detail::value_t::number_integer:
        case detail::value_t::number_float:
        case detail::value_t::binary:
        case detail::value_t::discarded:
        default: {
            return false;
        }
        }
    }

    return true;
}

std::vector<std::string> nlohmann::json_pointer::split(const std::string &reference_string)
{
    std::vector<std::string> result;

    if (reference_string.empty()) {
        return result;
    }

    if (reference_string[0] != '/') {
        throw(detail::parse_error::create(
            107, 1,
            detail::concat("JSON pointer must be empty or begin with '/' - was: '",
                           reference_string, "'")));
    }

    for (

        size_t slash = reference_string.find_first_of('/', 1),

               start = 1;

        start != 0;

        start = (slash == std::string::npos) ? 0 : slash + 1,

               slash = reference_string.find_first_of('/', start)) {
        auto reference_token = reference_string.substr(start, slash - start);

        for (size_t pos = reference_token.find_first_of('~'); pos != std::string::npos;
             pos = reference_token.find_first_of('~', pos + 1)) {
            if (pos == reference_token.size() - 1
                || (reference_token[pos + 1] != '0' && reference_token[pos + 1] != '1')) {
                throw(detail::parse_error::create(
                    108, 0, "escape character '~' must be followed with '0' or '1'"));
            }
        }

        detail::unescape(reference_token);
        result.push_back(reference_token);
    }

    return result;
}

void nlohmann::json_pointer::flatten(const std::string &reference_string,
                                     const ordered_json &value, ordered_json &result)
{
    switch (value.type()) {
    case detail::value_t::array: {
        if (value.m_data.m_value.array->empty()) {
            result[reference_string] = nullptr;
        } else {
            for (size_t i = 0; i < value.m_data.m_value.array->size(); ++i) {
                flatten(detail::concat(reference_string, '/', std::to_string(i)),
                        value.m_data.m_value.array->operator[](i), result);
            }
        }
        break;
    }

    case detail::value_t::object: {
        if (value.m_data.m_value.object->empty()) {
            result[reference_string] = nullptr;
        } else {
            for (const auto &element : *value.m_data.m_value.object) {
                flatten(detail::concat(reference_string, '/', detail::escape(element.first)),
                        element.second, result);
            }
        }
        break;
    }

    case detail::value_t::null:
    case detail::value_t::string:
    case detail::value_t::boolean:
    case detail::value_t::number_integer:
    case detail::value_t::number_float:
    case detail::value_t::binary:
    case detail::value_t::discarded:
    default: {
        result[reference_string] = value;
        break;
    }
    }
}

nlohmann::ordered_json nlohmann::json_pointer::unflatten(const ordered_json &value)
{
    if (!value.is_object()) {
        throw(detail::type_error::create(314, "only objects can be unflattened"));
    }

    ordered_json result;

    for (const auto &element : *value.m_data.m_value.object) {
        if (!element.second.is_primitive()) {
            throw(detail::type_error::create(315, "values in object must be primitive"));
        }

        json_pointer(element.first).get_and_create(result) = element.second;
    }

    return result;
}

std::strong_ordering nlohmann::json_pointer::operator<=>(const json_pointer &rhs) const noexcept
{
    return reference_tokens <=> rhs.reference_tokens;
}

bool nlohmann::json_pointer::operator==(const json_pointer &rhs) const noexcept
{
    return reference_tokens == rhs.reference_tokens;
}
