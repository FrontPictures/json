# Parsing and errors

## Text inputs

`json::parse()` accepts a `std::string`, a string literal, `std::string_view`, or
an input adapter. The provided adapters read `std::string` and
`std::vector<uint8_t>`, including iterator ranges for those containers.
Adapters borrow their input, so retain the source container while using one.

The callback-capable overload has the following signature.

```cpp
static json parse(const std::string &input,
                  parser_callback_t callback = nullptr,
                  bool allow_exceptions = true,
                  bool ignore_comments = false);
```

Parsing consumes a complete document. With `ignore_comments` enabled, C++-style
line and block comments are accepted. `accept(input, ignore_comments)` checks
syntax without constructing a DOM and returns a boolean.

```cpp
#include <nlohmann/json.hpp>
#include <cassert>
#include <string>

int main()
{
    using nlohmann::json;
    const std::string input = R"({"keep":1,"secret":2})";
    const auto filtered = json::parse(input, [](int, json::parse_event_t event, json &value) {
        return event != json::parse_event_t::key || value.get<std::string>() != "secret";
    });
    assert(filtered.contains("keep") && !filtered.contains("secret"));
    assert(json::accept(input));
    assert(json::parse("[", nullptr, false).is_discarded());

    bool caught = false;
    try {
        json::parse("[");
    } catch (const json::parse_error &error) {
        caught = error.id == 101 && error.byte > 0;
    }
    assert(caught);
}
```

Read a file into a string or byte vector before parsing. The compiled fork's
text API works with these in-memory inputs. The examples directory includes
standard stream usage for loading and saving them.

## Callbacks and SAX

`parser_callback_t` is a `std::function<bool(int, parse_event_t, json &)>`.
Events report object/array starts and ends, keys, and primitive values. The
integer argument is the nesting depth. Returning false filters a key or value,
or discards a container according to its event.

For streaming events from an in-memory buffer, implement `nlohmann::json_sax`
and call `json::sax_parse(input, &handler, format, strict, ignore_comments)`.
Include `<nlohmann/detail/input/json_sax.hpp>` for the interface. The handler
implements null, boolean, signed-integer, float, string, binary, container, key,
and error events. Its float callback also receives the source token for text
input. Returning false from an event stops parsing. Binary formats use the same
interface and their selected `input_format_t`.

## Exceptions

| Exception class | Typical cause |
| --- | --- |
| `json::parse_error` | Malformed JSON, a binary encoding, or a JSON Pointer. Carries a byte position. |
| `json::type_error` | Conversion or operation on an incompatible stored category. |
| `json::out_of_range` | Missing member, invalid index, excessive size, or a resource limit. |
| `json::invalid_iterator` | An iterator belongs to another container or an operation is invalid. |
| `json::other_error` | A failed JSON Patch test operation or another operation-specific error. |

Each derives from `json::exception`, provides `id`, and supplies a message through
`what()`. DOM parsing preserves the concrete exception type. Setting
`allow_exceptions=false` produces a discarded result for reported parse errors.
Allocation failures and exceptions thrown by application callbacks can still
propagate.

Long diagnostic tokens retain their first and last 4096 bytes with `...` between
them. Parsed string contents retain their full value. This bounds the extra
text kept for error reporting, including long trailing whitespace.
