cmake_minimum_required(VERSION 3.21)
project(json_documentation_examples LANGUAGES CXX)

set(JSON_BuildTests OFF)
set(JSON_Install OFF)
add_subdirectory(../.. json-library)
enable_testing()

foreach(name basic file_io binary serializer)
    add_executable(${name} ${name}.cpp)
    target_link_libraries(${name} PRIVATE nlohmann_json::nlohmann_json)
    add_test(NAME ${name} COMMAND ${name})
endforeach()
