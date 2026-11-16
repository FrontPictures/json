# Migration from upstream nlohmann/json

This fork starts from upstream 3.11.3 and uses a concrete compiled class with
fixed storage types. Use the API reference in this manual when updating callers.

| Upstream usage | Fork usage |
| --- | --- |
| Header-only inclusion or `single_include/` | Link `nlohmann_json::nlohmann_json`. The checkout's source include path is `src/`. |
| `basic_json<...>` specializations | Use the concrete `nlohmann::json` / `nlohmann::ordered_json` type. |
| Sorted `json` objects | Both names retain insertion order and compare objects in that order. |
| Custom string, allocator, container, serializer, or base-class template arguments | Use the fixed public storage types and explicit application conversions. |
| ADL `to_json` / `from_json`, `get_to`, and conversion macros | Construct object members explicitly and read them with `at()` / `get<T>()`. |
| Dedicated unsigned JSON category | Integer construction converts to `int64_t`. Text outside that range falls back to `double`. |
| `get_ptr<std::string *>()` | Call `get_ptr<std::string>()`. A category mismatch throws. |
| Generic container extraction | Use supported `get<T>()` types or `getArray<Container>()` with `reserve()` and `push_back()`. |
| Stream, `FILE*`, wide-string, or arbitrary iterator parsing | Load a string/byte vector, or use the provided adapters for those containers. |
| JSON stream insertion | Write `value.dump()` through the stream. |
| Library behavior configuration macros | Use the fixed behavior described here. CMake retains build/install/test options. |

Default construction remains null. Use `json::array()` and `json::object()` for
empty containers, and `json::array({value})` for a singleton array. Convert proxy
values such as a `std::vector<bool>` reference to `bool` explicitly.

Pretty output groups primitive array elements, applies a 20-byte threshold to
inline strings, and clamps indentation widths above 32. Update text fixtures
that assumed upstream's multiline array layout. `reserve_object_items(size_t)`
is available for the vector-backed object storage.

Application types can use explicit conversion functions.

```cpp
#include <nlohmann/json.hpp>
#include <cassert>
#include <string>

struct Display {
    std::string name;
    int port;
};

nlohmann::json encode(const Display &value)
{
    return nlohmann::json::object({{"name", value.name}, {"port", value.port}});
}

Display decode(const nlohmann::json &value)
{
    return {value.at("name").get<std::string>(), value.at("port").get<int>()};
}

int main()
{
    const auto display = decode(encode({"wall", 2}));
    assert(display.name == "wall" && display.port == 2);
}
```

The repository's `ChangeLog.md` retains the original upstream release history.
For the fork's subsequent fixes and changes, consult its Git history.
