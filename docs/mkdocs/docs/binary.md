# Binary formats

The library reads and writes CBOR, MessagePack, UBJSON, BJData, and BSON.
Each format has `json::to_format(value)` and `json::from_format(input)` functions.
Writers return `std::vector<uint8_t>` or append to a supplied output adapter.
Readers accept the provided string/byte-vector adapters. Object insertion order
is retained when writing and reading the fork's output.

```cpp
#include <nlohmann/json.hpp>
#include <cassert>
#include <cstdint>
#include <vector>

int main()
{
    using nlohmann::json;
    const json value = json::object({{"z", 1}, {"a", json::array({2, 3})}});
    const auto bytes = json::to_cbor(value);
    assert(json::from_cbor(bytes) == value);

    std::vector<uint8_t> message;
    json::to_msgpack(value, message);
    assert(json::from_msgpack(message) == value);

    json binary = json::binary({1, 2, 3}, 42);
    assert(binary.get_binary().has_subtype());
    assert(json::from_cbor(json::to_cbor(binary), true, true,
                           json::cbor_tag_handler_t::store) == binary);
}
```

## Reader and writer options

The `from_*` functions accept `strict=true` and `allow_exceptions=true` after the
input. Strict mode rejects trailing input. Disabling reported parse exceptions
returns a discarded result. `from_cbor()` additionally accepts a
`cbor_tag_handler_t` of `error`, `ignore`, or `store`. Read a tagged binary
value with `store` to retain its subtype. The default `error` mode rejects tags.

`to_ubjson()` and `to_bjdata()` accept `use_size=false` and `use_type=false`.
Type optimization requires size optimization. A call such as
`json::to_ubjson(value, true, true)` enables both.
BSON output expects an object at its root. Format-specific value ranges and
subtype rules still apply.

## Binary values

`json::binary(bytes)` stores bytes. Its optional second argument assigns a
`uint64_t` subtype. `get_binary()` returns the container, whose API includes
`has_subtype()`, `subtype()`, `set_subtype()`, and `clear_subtype()`. Each binary
format has its own representation of subtype metadata. JSON text dumping
represents the binary value as a `bytes`/`subtype` object.

## UBJSON expansion limits

A typed UBJSON array of nulls or booleans contains a count with no element
payload. Each such array is limited to 1,048,576 elements. Across one input,
the combined count is limited to 1,048,576 plus four times the bytes read so far.
Exceeding either limit reports `out_of_range` 408 through the SAX error handler
or the DOM parser. A DOM reader with `allow_exceptions=false` returns discarded.

The writer switches to an untyped representation when needed to keep its output
within the limits on typed arrays. Ordinary payload-bearing arrays retain their
normal format behavior. Numeric inputs follow the signed-storage policy in
[Values and conversions](values.md).
