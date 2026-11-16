# Text output

`dump()` returns a `std::string` containing compact JSON. Its parameters select
indentation, the indent character, Unicode escaping, and invalid-UTF-8 handling.

```cpp
std::string dump(int indent = -1,
                 char indent_char = ' ',
                 bool ensure_ascii = false,
                 error_handler_t error_handler = error_handler_t::strict) const;
```

A negative indentation selects compact output. Nonnegative indentation enables
pretty output. Values above 32 are clamped to 32 characters per nesting level.
Objects retain insertion order. `nlohmann::to_string(value)` returns `value.dump()`.

## Inline arrays

Pretty output groups numbers, booleans, nulls, strings of at most 20 UTF-8 bytes,
and empty arrays and objects on the same line. Longer strings, nonempty arrays,
nonempty objects, and binary values start an indented line. After such an
element, a following group of short values starts on an indented line too.

```json
["short", 1, true]
```

An array containing a nonempty object has this layout with indentation two.

```json
[1,
  {
    "x": 2
  },
  3
]
```

The string threshold measures bytes before JSON escaping. Ten copies of `é`
occupy 20 UTF-8 bytes and stay inline, while eleven occupy 22 bytes and start a
new line. Multiple short values can produce a long line.

## Unicode and special values

`ensure_ascii=true` escapes non-ASCII code points. Strings are checked for valid
UTF-8. `strict` throws `type_error` 316 for malformed input, `ignore` drops invalid
bytes, and `replace` substitutes U+FFFD. These modes are members of
`json::error_handler_t`.

NaN and infinities serialize as `null`. Discarded values also serialize as
`null`. Binary values serialize as objects containing `bytes` and `subtype`.
Use the binary format functions to retain the binary category through a roundtrip.

## Serializer helpers

`nlohmann::detail::serializer` is exposed for incremental output through an
output adapter. Include its header and the output adapters header explicitly.
Keep the output container alive until the serializer finishes.

```cpp
#include <nlohmann/json.hpp>
#include <nlohmann/detail/output/output_adapters.hpp>
#include <nlohmann/detail/output/serializer.hpp>
#include <cassert>
#include <string>

int main()
{
    std::string output;
    nlohmann::detail::serializer writer(nlohmann::detail::output_adapter(output), ' ');
    const auto indent = writer.dumpObjectBegin(true, 2, 0, true);
    writer.dumpKey("values", true, indent, false, true);
    writer.dump(nlohmann::json::array({1, 2}), true, false, 2);
    writer.dumpObjectEnd(true, 0, true);
    assert(output == R"({"values": [1, 2]})");
}
```

The final `oneLiner` argument of `dumpObjectBegin`, `dumpKey`,
`dumpObjectComa`, and `dumpObjectEnd` selects inline object formatting. Their
spelling follows the existing API, including `Coma`. Array helpers are
`dumpArrayBegin`, `dumpArrayPrettyIndent`, `dumpArrayComa`, and `dumpArrayEnd`.
Scalar helpers include `dumpString`, `dumpBool`, `dump_integer`, `dump_float`,
and `dumpNull`, with `dumpEmptyObject` and `dumpEmptyArray` for empty containers.
The caller supplies balanced delimiters, commas, and matching indentation state.
The 32-character clamp belongs to `json::dump()`.
