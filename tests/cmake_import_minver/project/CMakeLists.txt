cmake_minimum_required(VERSION 3.21)

project(DummyImportMinVer CXX)

find_package(nlohmann_json 1.0.0 REQUIRED)

add_executable(with_namespace_target main.cpp)
target_link_libraries(with_namespace_target nlohmann_json::nlohmann_json)
