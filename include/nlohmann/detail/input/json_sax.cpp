#include "json_sax.hpp"
#include "nlohmann/json.hpp"
#include "nlohmann/detail/string_concat.hpp"

nlohmann::detail::json_sax_dom_callback_parser::json_sax_dom_callback_parser(
    ordered_json &r, const parser_callback_t cb, const bool allow_exceptions_)
    : root(r), callback(cb), allow_exceptions(allow_exceptions_),
      discarded(new ordered_json(detail::value_t::discarded))
{
    keep_stack.push_back(true);
}

nlohmann::detail::json_sax_dom_callback_parser::~json_sax_dom_callback_parser() = default;

bool nlohmann::detail::json_sax_dom_callback_parser::null()
{
    handle_value(nullptr);
    return true;
}

bool nlohmann::detail::json_sax_dom_callback_parser::boolean(bool val)
{
    handle_value(val);
    return true;
}

bool nlohmann::detail::json_sax_dom_callback_parser::number_integer(int64_t val)
{
    handle_value(val);
    return true;
}

bool nlohmann::detail::json_sax_dom_callback_parser::number_float(double val, const std::string &)
{
    handle_value(val);
    return true;
}

bool nlohmann::detail::json_sax_dom_callback_parser::string(std::string &val)
{
    handle_value(val);
    return true;
}

bool nlohmann::detail::json_sax_dom_callback_parser::binary(byte_container_with_subtype &val)
{
    handle_value(std::move(val));
    return true;
}

bool nlohmann::detail::json_sax_dom_callback_parser::start_object(size_t len)
{
    const bool keep = callback(static_cast<int>(ref_stack.size()), parse_event_t::object_start,
                               *discarded);
    keep_stack.push_back(keep);

    auto val = handle_value(ordered_json::value_t::object, true);
    ref_stack.push_back(val.second);

    if (ref_stack.back() && len != static_cast<size_t>(-1) && len > ref_stack.back()->max_size()) {
        throw(out_of_range::create(408, concat("excessive object size: ", std::to_string(len))));
    }

    return true;
}

bool nlohmann::detail::json_sax_dom_callback_parser::key(std::string &val)
{
    ordered_json k = ordered_json(val);

    const bool keep = callback(static_cast<int>(ref_stack.size()), parse_event_t::key, k);
    key_keep_stack.push_back(keep);

    if (keep && ref_stack.back()) {
        object_element = &(ref_stack.back()->m_data.m_value.object->operator[](val) = *discarded);
    }

    return true;
}

bool nlohmann::detail::json_sax_dom_callback_parser::end_object()
{
    if (ref_stack.back()) {
        if (!callback(static_cast<int>(ref_stack.size()) - 1, parse_event_t::object_end,
                      *ref_stack.back())) {
            *ref_stack.back() = *discarded;
        }
    }

    ref_stack.pop_back();
    keep_stack.pop_back();

    if (!ref_stack.empty() && ref_stack.back() && ref_stack.back()->is_structured()) {
        for (auto it = ref_stack.back()->begin(); it != ref_stack.back()->end(); ++it) {
            if (it->is_discarded()) {
                ref_stack.back()->erase(it);
                break;
            }
        }
    }

    return true;
}

bool nlohmann::detail::json_sax_dom_callback_parser::start_array(size_t len)
{
    const bool keep = callback(static_cast<int>(ref_stack.size()), parse_event_t::array_start,
                               *discarded);
    keep_stack.push_back(keep);

    auto val = handle_value(detail::value_t::array, true);
    ref_stack.push_back(val.second);

    if (ref_stack.back() && len != static_cast<size_t>(-1) && len > ref_stack.back()->max_size()) {
        throw(out_of_range::create(408, concat("excessive array size: ", std::to_string(len))));
    }

    return true;
}

bool nlohmann::detail::json_sax_dom_callback_parser::end_array()
{
    bool keep = true;

    if (ref_stack.back()) {
        keep = callback(static_cast<int>(ref_stack.size()) - 1, parse_event_t::array_end,
                        *ref_stack.back());
        if (!keep) {
            *ref_stack.back() = *discarded;
        }
    }

    ref_stack.pop_back();
    keep_stack.pop_back();

    if (!keep && !ref_stack.empty() && ref_stack.back()->is_array()) {
        ref_stack.back()->m_data.m_value.array->pop_back();
    }

    return true;
}

bool nlohmann::detail::json_sax_dom_callback_parser::parse_error(size_t, const std::string &,
                                                                 const exception &ex)
{
    errored = true;
    static_cast<void>(ex);
    if (allow_exceptions) {
        throw(ex);
    }
    return false;
}

std::pair<bool, nlohmann::ordered_json *>
nlohmann::detail::json_sax_dom_callback_parser::handle_value(ordered_json &&v,
                                                             const bool skip_callback)
{
    if (!keep_stack.back()) {
        return {false, nullptr};
    }

    auto value = ordered_json(std::move(v));

    const bool keep = skip_callback
                      || callback(static_cast<int>(ref_stack.size()), parse_event_t::value, value);

    if (!keep) {
        return {false, nullptr};
    }

    if (ref_stack.empty()) {
        root = std::move(value);
        return {true, &root};
    }

    if (!ref_stack.back()) {
        return {false, nullptr};
    }

    if (ref_stack.back()->is_array()) {
        ref_stack.back()->m_data.m_value.array->emplace_back(std::move(value));
        return {true, &(ref_stack.back()->m_data.m_value.array->back())};
    }

    const bool store_element = key_keep_stack.back();
    key_keep_stack.pop_back();

    if (!store_element) {
        return {false, nullptr};
    }

    *object_element = std::move(value);
    return {true, object_element};
}

nlohmann::detail::json_sax_dom_parser::json_sax_dom_parser(ordered_json &r,
                                                           const bool allow_exceptions_)
    : root(r), allow_exceptions(allow_exceptions_)
{}

bool nlohmann::detail::json_sax_dom_parser::null()
{
    handle_value(nullptr);
    return true;
}

bool nlohmann::detail::json_sax_dom_parser::boolean(bool val)
{
    handle_value(val);
    return true;
}

bool nlohmann::detail::json_sax_dom_parser::number_integer(int64_t val)
{
    handle_value(val);
    return true;
}

bool nlohmann::detail::json_sax_dom_parser::number_float(double val, const std::string &)
{
    handle_value(val);
    return true;
}

bool nlohmann::detail::json_sax_dom_parser::string(std::string &val)
{
    handle_value(val);
    return true;
}

bool nlohmann::detail::json_sax_dom_parser::binary(byte_container_with_subtype &val)
{
    handle_value(std::move(val));
    return true;
}

bool nlohmann::detail::json_sax_dom_parser::start_object(size_t len)
{
    ref_stack.push_back(handle_value(ordered_json::value_t::object));

    if (len != static_cast<size_t>(-1) && len > ref_stack.back()->max_size()) {
        throw(out_of_range::create(408, concat("excessive object size: ", std::to_string(len))));
    }

    return true;
}

bool nlohmann::detail::json_sax_dom_parser::key(std::string &val)
{
    object_element = &(ref_stack.back()->m_data.m_value.object->operator[](val));
    return true;
}

bool nlohmann::detail::json_sax_dom_parser::end_object()
{
    ref_stack.pop_back();
    return true;
}

bool nlohmann::detail::json_sax_dom_parser::start_array(size_t len)
{
    ref_stack.push_back(handle_value(ordered_json::value_t::array));

    if (len != static_cast<size_t>(-1) && len > ref_stack.back()->max_size()) {
        throw(out_of_range::create(408, concat("excessive array size: ", std::to_string(len))));
    }

    return true;
}

bool nlohmann::detail::json_sax_dom_parser::end_array()
{
    ref_stack.pop_back();
    return true;
}

bool nlohmann::detail::json_sax_dom_parser::parse_error(size_t, const std::string &,
                                                        const exception &ex)
{
    errored = true;
    static_cast<void>(ex);
    if (allow_exceptions) {
        throw(ex);
    }
    return false;
}

nlohmann::ordered_json *nlohmann::detail::json_sax_dom_parser::handle_value(ordered_json &&v)
{
    if (ref_stack.empty()) {
        root = v;
        return &root;
    }

    if (ref_stack.back()->is_array()) {
        ref_stack.back()->m_data.m_value.array->emplace_back(std::move(v));
        return &(ref_stack.back()->m_data.m_value.array->back());
    }

    *object_element = ordered_json(std::move(v));
    return object_element;
}
