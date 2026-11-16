# Pointers and patches

## JSON Pointer

`json::json_pointer` and `nlohmann::json_pointer` name the same concrete pointer
type. The empty string addresses the root. Slash-separated tokens address
object keys or array indices. A key's `~` and `/` are escaped as `~0` and `~1`.
Use `at(pointer)` for checked access and `contains(pointer)` to check presence.
Mutable `operator[](pointer)` can create missing structure.

```cpp
#include <nlohmann/json.hpp>
#include <cassert>

int main()
{
    using nlohmann::json;
    json document = json::object({{"ports", json::array({1, 2})}});
    const json::json_pointer path("/ports/1");
    assert(document.at(path).get<int>() == 2);
    document[path] = 3;

    const auto patch = json::array({json::object({
        {"op", "replace"}, {"path", "/ports/0"}, {"value", 7}
    })});
    const auto changed = document.patch(patch);
    assert(changed.at(json::json_pointer("/ports/0")).get<int>() == 7);
    assert(document.at(json::json_pointer("/ports/0")).get<int>() == 1);

    json settings = json::object({{"keep", 1}, {"remove", 2}});
    settings.merge_patch(json::object({{"remove", nullptr}, {"added", true}}));
    assert(!settings.contains("remove") && settings.at("added").get<bool>());
}
```

Pointers support `to_string()`, string conversion, `/`, `/=`, `parent_pointer()`,
`back()`, `push_back()`, `pop_back()`, `empty()`, equality, and ordering.
The `_json_pointer` literal constructs a pointer. The `_json` literal parses JSON.
Both literals are available globally and in `nlohmann::literals`.

## JSON Patch and Merge Patch

`patch(patch_document)` copies the current value and applies JSON Patch
operations to that copy. `patch_inplace()` mutates the current value in operation
order. If a later operation fails, earlier changes can remain.
`json::diff(source, target, path)` generates a patch document.

The `test` operation uses the fork's JSON equality. Object member order therefore
participates in a test comparison. Insertion order also participates in direct
object equality and comparison of patch results.

`merge_patch()` applies JSON Merge Patch. An object patch recursively updates
object members, and a null member removes the corresponding key. A non-object
patch replaces the target value.

`flatten()` returns an object whose keys are JSON Pointer paths to leaves.
`unflatten()` reconstructs nested structure from such an object. Empty containers
are represented by null leaves during flattening.
