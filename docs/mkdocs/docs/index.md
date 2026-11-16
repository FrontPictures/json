# Simplified JSON for C++

This manual describes version 1.0.0 of the compiled FrontPictures fork, based on
nlohmann/json 3.11.3.
Its concrete `nlohmann::ordered_json` class stores objects in insertion order.
`nlohmann::json` names the same class. The library uses C++20 and CMake 3.21 or
newer. Headers and implementation files live together in `src/nlohmann/`.

```cpp
#include <nlohmann/json.hpp>
#include <cassert>
#include <string>

int main()
{
    using nlohmann::json;
    json document = json::object({{"name", "display"}, {"ports", json::array({1, 2})}});
    document["enabled"] = true;
    assert(document.at("name").get<std::string>() == "display");
    assert(json::parse(document.dump()) == document);
}
```

Link the CMake target `nlohmann_json::nlohmann_json` to compile and link a
consumer. Include `<nlohmann/json_fwd.hpp>` when a declaration is sufficient,
and `<nlohmann/json.hpp>` when constructing or accessing values.

## Manual

- Build the library, install it, and run its tests with [CMake](cmake.md).
- Construct values and inspect their fixed storage types in [Values and conversions](values.md).
- Access elements, reserve objects, and traverse containers with [Containers and iteration](containers.md).
- Parse strings, filter values, and handle failures with [Parsing and errors](parsing.md).
- Control indentation, Unicode escaping, and inline arrays with [Text output](serialization.md).
- Read and write the five supported [Binary formats](binary.md).
- Address and transform values with [Pointers and patches](pointers.md).
- Look up method groups in the [API reference](api.md).
- Update existing upstream callers with the [Migration guide](migration.md).

The original release history is retained in the repository's `ChangeLog.md`.
The original project and its manuals describe additional template-based APIs.
This manual and the fork's headers describe the supported interface here.

Original project and license information are available from
[nlohmann/json](https://github.com/nlohmann/json).
