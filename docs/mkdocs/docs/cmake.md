# CMake

## Build a consumer

The source checkout contains headers and compiled implementation files under
`src/nlohmann/`. Add the library directory and link its target.

```cmake
cmake_minimum_required(VERSION 3.21)
project(my_app LANGUAGES CXX)

add_subdirectory(path/to/json)
add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE nlohmann_json::nlohmann_json)
```

The target supplies the `src/` include directory, the C++20 requirement, and the
static library. Write `#include <nlohmann/json.hpp>` in the consumer. An explicit
source include directory, when needed, must point to the checkout's `src/`.
Link the library even when including its headers directly.

FetchContent can use the same checkout or a pinned revision of the fork.

```cmake
include(FetchContent)
FetchContent_Declare(json
    GIT_REPOSITORY https://github.com/FrontPictures/json.git
    GIT_TAG <commit-sha>)
FetchContent_MakeAvailable(json)
target_link_libraries(my_app PRIVATE nlohmann_json::nlohmann_json)
```

Replace `<commit-sha>` with the revision required by the application.

## Install and import

Run these commands from the repository root.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DJSON_BuildTests=OFF -DJSON_Install=ON
cmake --build build --config Release --parallel 2
cmake --install build --config Release --prefix /path/to/json-install
```

The prefix contains the library, headers beneath `include/nlohmann/`, and CMake
package files. Source `.cpp` files are excluded from the installed headers.
Installed consumers use the exported target.

```cmake
find_package(nlohmann_json CONFIG REQUIRED)
add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE nlohmann_json::nlohmann_json)
```

Pass `-DCMAKE_PREFIX_PATH=/path/to/json-install` when configuring the consumer.
Use a matching architecture and compatible compiler/runtime for the static
library. For multi-configuration generators, select the intended configuration
with `--config Release` and `ctest -C Release`.

## Options

| Variable | Default | Purpose |
| --- | --- | --- |
| `JSON_BuildTests` | `ON` for a top-level build, otherwise `OFF` | Build the test executables and CMake integration checks. |
| `JSON_Install` | `ON` for a top-level build, otherwise `OFF` | Generate installation rules and export package targets. |
| `JSON_SystemInclude` | `OFF` | Mark the target's include directories as system directories. |
| `JSON_FastTests` | `OFF` | Skip test cases marked expensive. Available when tests are enabled. |
| `JSON_Valgrind` | `OFF` | Run tests through an installed Valgrind executable. |
| `JSON_TestDataDirectory` | Unset | Use an existing checkout of the upstream v3.1.0 test data. |

Library behavior is fixed by its API. The old library configuration macros and
single-header build selection have been removed.

## Tests and examples

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DJSON_BuildTests=ON
cmake --build build --config Release --parallel 2
ctest --test-dir build -C Release --parallel 2 --output-on-failure
```

CTest downloads test fixtures on the first run unless `JSON_TestDataDirectory`
is supplied. Git and network access are needed for that download. The full run
includes exhaustive Unicode checks. Set `-DJSON_FastTests=ON` for a shorter run.

The documentation examples have their own consumer project.

```sh
cmake -S docs/examples -B build/docs-examples -DCMAKE_BUILD_TYPE=Release
cmake --build build/docs-examples --config Release --parallel 2
ctest --test-dir build/docs-examples -C Release --output-on-failure
```
