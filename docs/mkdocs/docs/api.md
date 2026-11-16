# API reference

The public headers are `<nlohmann/json.hpp>` and `<nlohmann/json_fwd.hpp>`.
`json` aliases the concrete `ordered_json` class. Include the complete header to
construct values, call methods, and use JSON Pointer or the literal operators.
The lists below group the current interface. Parameter defaults and exact
signatures are declared in `src/nlohmann/json.hpp` in the checkout.

## Types and construction

| Area | Names |
| --- | --- |
| Values | `value_t`, `object_t`, `array_t`, `string_t`, `boolean_t`, `number_integer_t`, `number_float_t`, `binary_t` |
| Errors | `exception`, `parse_error`, `type_error`, `out_of_range`, `invalid_iterator`, `other_error` |
| Iteration | `iterator`, `const_iterator`, `reverse_iterator`, `const_reverse_iterator` |
| Parsing | `parse_event_t`, `parser_callback_t`, `input_format_t`, `cbor_tag_handler_t`, `json_sax_t` |
| Output | `error_handler_t` |
| Factories | `array()`, `object()`, `binary()` |
| Metadata | `meta()` |

Constructors cover null, scalar values, enums, strings, binary containers, JSON
arrays and objects, compatible sequence containers, initializer lists, repeated
array values, and JSON iterator ranges. Copy and move construction and assignment
are provided. Prefer explicit factories when selecting an empty or singleton
container's category.

## Inspection, conversion, and access

| Area | Methods |
| --- | --- |
| Category | `type()`, `type_name()`, `is_null()`, `is_boolean()`, `is_number()`, `is_number_integer()`, `is_number_float()`, `is_object()`, `is_array()`, `is_string()`, `is_binary()`, `is_discarded()`, `is_primitive()`, `is_structured()` |
| Copies | `get<T>()`, `getArray<Container>()` |
| Storage access | `get_ref<T>()`, `get_ptr<T>()`, `get_string_ref()`, `get_binary()` |
| Elements | `at()`, `operator[]`, `value()`, `front()`, `back()` |
| Lookup | `find()`, `count()`, `contains()` |
| Capacity | `size()`, `empty()`, `max_size()`, `reserve_object_items()` |

Conversion operators expose `value_t`, explicit `bool`, explicit `int`, explicit
`double`, and `std::string`. The explicit scalar conversions delegate to `get<T>()`.

## Iteration, changes, and comparison

| Area | Methods |
| --- | --- |
| Forward | `begin()`, `end()`, `cbegin()`, `cend()`, `items()` |
| Reverse | `rbegin()`, `rend()`, `crbegin()`, `crend()` |
| Append/insert | `push_back()`, `operator+=`, `emplace_back()`, `emplace()`, `insert()`, `insert_iterator()` |
| Other changes | `erase()`, `update()`, `clear()`, `swap()` |
| Comparison | `operator==`, `operator!=`, `operator<=>`, and relational operators supplied by comparison rewriting |

`insert_iterator()` is the low-level array insertion helper. Its caller must
provide an array and a position iterator belonging to that array. The checked
`insert()` overloads supply the usual container validation.

Objects compare in insertion order. Integer/float comparison converts the integer
to double. NaN produces unordered numeric comparisons. Type ordering places null,
booleans, numbers, objects, arrays, strings, and binary values in that order.
Discarded values have special unordered comparison behavior.

## Parsing, serialization, and transformations

| Area | Methods |
| --- | --- |
| Text | `parse()`, `accept()`, `sax_parse()`, `dump()` |
| Binary readers | `from_cbor()`, `from_msgpack()`, `from_ubjson()`, `from_bjdata()`, `from_bson()` |
| Binary writers | `to_cbor()`, `to_msgpack()`, `to_ubjson()`, `to_bjdata()`, `to_bson()` |
| Pointers | `json_pointer`, pointer overloads of `at()`, `operator[]`, `contains()`, and `value()` |
| Transformations | `flatten()`, `unflatten()`, `patch()`, `patch_inplace()`, `diff()`, `merge_patch()` |

The namespace also provides `to_string()`, `_json`, and `_json_pointer`.
`std::hash<nlohmann::ordered_json>` is supplied. Binary values use the concrete
`nlohmann::byte_container_with_subtype` type. Incremental formatting uses the
serializer helpers documented in [Text output](serialization.md).
