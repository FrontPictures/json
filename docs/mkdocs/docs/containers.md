# Containers and iteration

## Element access

`at(key)`, `at(index)`, and `at(json_pointer)` check that the requested element
exists. Missing keys and out-of-range indices throw `out_of_range`.

Mutable object `operator[]` inserts a missing key with a null value. A null
parent becomes an object when accessed by key, or an array when accessed by
index. Mutable array access grows the array with null values as needed.
For const access, use `at()` when the element's presence is uncertain.

`value(key, default_value)` and `value(pointer, default_value)` return a converted
value or the default when the element is missing. The default's C++ type selects
the conversion. String-literal defaults return `std::string`. An existing value
of an incompatible type throws. The parent for `value()` must be an object.

```cpp
#include <nlohmann/json.hpp>
#include <cassert>
#include <string>

int main()
{
    using nlohmann::json;
    json object = json::object();
    object.reserve_object_items(16);
    object["z"] = 1;
    object["a"] = 2;
    assert(object.value("missing", 42) == 42);
    assert(object.contains("z"));
    assert(object.find("a") != object.end());

    std::string keys;
    for (auto it = object.begin(); it != object.end(); ++it) {
        keys += it.key();
        assert(it.value().is_number_integer());
    }
    assert(keys == "za");
}
```

## Ordered objects

Both `json` and `ordered_json` retain insertion order during iteration and
serialization. Equality of objects includes their member order. Two objects
with the same key/value pairs inserted in different orders can compare unequal.

Objects use a vector-backed map with linear key lookup. Building a very wide
object by repeated insertion can therefore take quadratic time.
`reserve_object_items(size_t)` reserves storage on an existing object. It
preserves its size, contents, and order and throws `type_error` for another
category. Reservation reduces reallocations and retains linear key lookup.

Reallocation invalidates references, pointers, and iterators into an object or
array. Erasing elements can also invalidate access to subsequent elements.
Choose capacity before keeping references into a growing object.

## Traversal and modification

Range-based iteration visits values. `begin()`/`end()` and their const and
reverse variants expose iterators. An object's iterator provides `key()` and
`value()`. Array iterators support positional arithmetic. Arithmetic and ordering
operations restricted by the container can throw `invalid_iterator`.

`items()` yields key/value proxies. Object keys are stored member names. Array
keys are decimal index strings. Its iterators advertise forward traversal.
Value-initialized JSON iterators can be compared with each other.

`push_back()`, `emplace_back()`, and `operator+=` append array values. `emplace()`
inserts object members. `insert()` supports array positions and object iterator
ranges. `erase()` removes a key, index, or iterator range. `update()` replaces or
adds object members, with optional recursive object merging. `swap()` exchanges
JSON values or supported underlying containers.

`size()` counts object members and array elements. Null has size zero and other
primitive values have size one. Use `get_string_ref().size()` for string bytes
and `get_binary().size()` for binary bytes. `empty()` reports null and empty
structured containers as empty. `clear()` empties containers and strings and
resets booleans and numbers to zero while preserving their category.
