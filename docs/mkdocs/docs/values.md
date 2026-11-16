# Values and conversions

## Fixed storage

| JSON category | Public storage type |
| --- | --- |
| Object | `json::object_t`, an `ordered_map<std::string, json>` backed by a vector |
| Array | `json::array_t`, a `std::vector<json>` |
| String | `json::string_t`, a UTF-8 `std::string` |
| Boolean | `json::boolean_t`, `bool` |
| Integer | `json::number_integer_t`, `int64_t` |
| Floating point | `json::number_float_t`, `double` |
| Binary | `json::binary_t`, `byte_container_with_subtype` |

Null and discarded values have their own `value_t` tags. Use `type()`,
`type_name()`, and the `is_*()` predicates to inspect the category.
`is_number()` covers integers and floating-point numbers. `is_structured()`
covers arrays and objects.

## Construction

Default construction creates null. Construct from booleans, integers, enums,
floating-point values, strings, binary containers, arrays, and `object_t`.
`std::string_view` construction copies the string's contents. Compatible sequence
containers can construct arrays when their elements can construct JSON values.

Use `json::array({...})` and `json::object({...})` to make container intent explicit,
particularly for empty containers or singleton arrays. Object initializer lists
contain key/value pairs. Repeated initializer-list keys retain the first value.

```cpp
#include <nlohmann/json.hpp>
#include <cassert>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>

int main()
{
    using nlohmann::json;
    json values = json::array({1, 2, 3});
    const auto integers = values.get<std::vector<int>>();
    const auto doubles = values.getArray<std::vector<double>>();
    assert(integers.size() == 3 && doubles[2] == 3.0);

    json text = std::string("display");
    assert(text.get_ref<const std::string &>() == "display");
    assert(*text.get_ptr<std::string>() == "display");

    const auto maximum = (std::numeric_limits<uint64_t>::max)();
    json integer = maximum;
    assert(integer.get<int64_t>() == -1);
    assert(integer.get<uint64_t>() == maximum);
}
```

## Integer policy

Integer construction deliberately converts to `int64_t`, including unsigned
inputs and enums. On supported platforms, constructing from `UINT64_MAX` stores
`-1`. Reading that stored integer as `uint64_t` returns `UINT64_MAX`.

Text parsing first attempts a signed integer. Decimal integers outside the
`int64_t` range fall back to `double`, which can lose precision. Binary formats
can carry unsigned integer bits that become a signed stored value. Consequently,
text and binary encodings of an unsigned value above `INT64_MAX` can yield
different stored categories and values.

`get<T>()` converts integral destinations through `int64_t`. Integer extraction
also accepts booleans as zero or one and floating-point values by conversion.
Narrowing uses the corresponding C++ cast. Floating-to-integer conversion
requires a finite input within the destination's representable range.
`get<double>()` and `get<float>()` accept stored numbers. `get<bool>()` accepts a
stored boolean.

## Copies, references, and pointers

`get<T>()` returns a copy. The exact storage types above are supported, together
with integral and enum conversions, `float`, `json`, `std::vector<int>`,
`std::vector<float>`, and `std::vector<std::string>`.

`getArray<Container>()` converts each element of an array through
`get<Container::value_type>()`. Its destination needs default construction,
`reserve()`, and `push_back()`.

`get_ref<T>()` and `get_ptr<T>()` access the exact stored representation. Use
storage types such as `json::array_t`, `int64_t`, `double`, or `std::string`.
`get_ptr<T>()` takes a pointee type, as in `get_ptr<std::string>()`. A mismatched
stored category throws `type_error` 302. References and pointers follow the
invalidation rules of their owning value and container.

`get_string_ref()` returns a const string reference. `get_binary()` provides
mutable and const access to a binary container. These accessors throw on an
incompatible category.
