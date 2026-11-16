# Simplified JSON for C++

This fork of nlohmann/json 3.11.3 provides a compiled library with one concrete
`nlohmann::ordered_json` class. `nlohmann::json` is an alias for that class.
Objects retain insertion order. The public build requirement is C++20.

## CMake

```cmake
add_subdirectory(path/to/json)
target_link_libraries(my_app PRIVATE nlohmann_json::nlohmann_json)
```

The target propagates its include path, C++20 requirement, and static library.
Installed packages can be used with `find_package(nlohmann_json CONFIG REQUIRED)`.
Include `<nlohmann/json.hpp>`, or `<nlohmann/json_fwd.hpp>` for declarations only.
There is no header-only distribution or macro-based library configuration.

```sh
cmake -S . -B build -DJSON_BuildTests=ON
cmake --build build -j
ctest --test-dir build --output-on-failure
```

Tests download the upstream test data on their first run. Use
`JSON_TestDataDirectory` to point to an existing copy.

## Differences from upstream

- Storage types are fixed: insertion-ordered objects, vectors, UTF-8 strings,
  booleans, `int64_t`, `double`, and binary values. Custom allocators, custom
  string types, ADL serializers, and user-defined conversion macros are absent.
- Integer construction deliberately converts to `int64_t`, including unsigned
  inputs. On supported platforms `json(UINT64_MAX)` stores `-1`; converting it
  back with `get<uint64_t>()` yields `UINT64_MAX`. Parsing a decimal integer
  outside the signed range falls back to `double` and can lose precision.
- `get_ptr<T>()` takes the pointee type and throws on a type mismatch.
- `reserve_object_items(size_t)` reserves object capacity without changing
  contents or insertion order. It throws for values that are not objects.
- `dump()` produces compact JSON by default. Nonnegative indentation enables
  pretty output; indentation widths above 32 are clamped to 32. Primitive array
  values share a line. Strings of at most 20 UTF-8 bytes, empty arrays, and empty
  objects also stay inline; longer strings and nonempty containers start an
  indented line. The string threshold is measured before JSON escaping.
- Discarded values serialize as `null`.

The upstream `docs/` tree and changelog describe the original library and may
include APIs unavailable in this fork. See the headers and tests for the fork's
supported API. Original project: https://github.com/nlohmann/json.

Licensed under MIT; see [LICENSE.MIT](LICENSE.MIT).
