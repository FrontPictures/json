#ifndef PARSER_TYPES_H
#define PARSER_TYPES_H

#include <cstdint>
#include <functional>

namespace nlohmann {
class ordered_json;
namespace detail {

enum class parse_event_t : uint8_t {
    object_start,
    object_end,
    array_start,
    array_end,
    key,
    value
};

using parser_callback_t
    = std::function<bool(int /*depth*/, parse_event_t /*event*/, ordered_json & /*parsed*/)>;
} // namespace detail
} // namespace nlohmann

#endif
