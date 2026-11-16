# Contributing

This repository maintains a compiled fork of nlohmann/json. Library headers and
implementation files live in `src/nlohmann/`. Edit those files directly and link
the CMake target when building consumers.

## Build and verify

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DJSON_BuildTests=ON
cmake --build build --config Release --parallel 2
ctest --test-dir build -C Release --parallel 2 --output-on-failure
```

Use `JSON_FastTests=ON` to skip test cases marked expensive. The full suite
includes binary fixture roundtrips, exhaustive Unicode validation, and CMake
consumer/install tests. Adapt or add a focused regression test for a behavior
change. Preserve the concrete public API and avoid adding C++20-only constructs
when a C++17-compatible implementation works.

Update the manual and examples when changing documented behavior. Instructions
for building the site and compiling the examples are in
[the documentation guide](../docs/README.md).

The GitHub Actions matrix checks Ubuntu, macOS, and Windows. Compile errors and
failing test output are included in job summaries and check annotations.
