cmake_minimum_required(VERSION 3.21)

##
## PROJECT
## name and version
##
project(nlohmann_json VERSION 3.11.3 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

##
## MAIN_PROJECT CHECK
## determine if nlohmann_json is built as a subproject (using add_subdirectory) or if it is the main project
##
set(MAIN_PROJECT OFF)
if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(MAIN_PROJECT ON)
endif()

##
## INCLUDE
##
##
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})

##
## OPTIONS
##

option(JSON_BuildTests "Build the unit tests." ${MAIN_PROJECT})
option(JSON_Install "Install CMake targets." ${MAIN_PROJECT})
option(JSON_SystemInclude "Include as system headers." OFF)

##
## CONFIGURATION
##
include(GNUInstallDirs)

set(NLOHMANN_JSON_TARGET_NAME               ${PROJECT_NAME})
set(NLOHMANN_JSON_CONFIG_INSTALL_DIR        "${CMAKE_INSTALL_DATADIR}/cmake/${PROJECT_NAME}" CACHE INTERNAL "")
set(NLOHMANN_JSON_INCLUDE_INSTALL_DIR       "${CMAKE_INSTALL_INCLUDEDIR}")
set(NLOHMANN_JSON_TARGETS_EXPORT_NAME       "${PROJECT_NAME}Targets")
set(NLOHMANN_JSON_CMAKE_CONFIG_TEMPLATE     "cmake/config.cmake.in")
set(NLOHMANN_JSON_CMAKE_CONFIG_DIR          "${CMAKE_CURRENT_BINARY_DIR}")
set(NLOHMANN_JSON_CMAKE_VERSION_CONFIG_FILE "${NLOHMANN_JSON_CMAKE_CONFIG_DIR}/${PROJECT_NAME}ConfigVersion.cmake")
set(NLOHMANN_JSON_CMAKE_PROJECT_CONFIG_FILE "${NLOHMANN_JSON_CMAKE_CONFIG_DIR}/${PROJECT_NAME}Config.cmake")
set(NLOHMANN_JSON_CMAKE_PROJECT_TARGETS_FILE "${NLOHMANN_JSON_CMAKE_CONFIG_DIR}/${PROJECT_NAME}Targets.cmake")

set(NLOHMANN_JSON_INCLUDE_BUILD_DIR "${PROJECT_SOURCE_DIR}/src/")

if (JSON_SystemInclude)
    set(NLOHMANN_JSON_SYSTEM_INCLUDE "SYSTEM")
endif()

##
## TARGET
## create target and add include path
##
add_library(${NLOHMANN_JSON_TARGET_NAME} STATIC
    src/nlohmann/detail/conversions/diyfp.h src/nlohmann/detail/conversions/diyfp.cpp
    src/nlohmann/detail/conversions/grisu2.h src/nlohmann/detail/conversions/grisu2.cpp
    src/nlohmann/detail/conversions/to_chars.hpp src/nlohmann/detail/conversions/to_chars.cpp
    src/nlohmann/detail/input/binary_reader.hpp src/nlohmann/detail/input/binary_reader.cpp
    src/nlohmann/detail/input/input_adapters.hpp src/nlohmann/detail/input/input_adapters.cpp
    src/nlohmann/detail/input/input_format_t.h src/nlohmann/detail/input/input_format_t.cpp
    src/nlohmann/detail/input/json_sax.hpp src/nlohmann/detail/input/json_sax.cpp
    src/nlohmann/detail/input/lexer.hpp src/nlohmann/detail/input/lexer.cpp
    src/nlohmann/detail/input/parser.hpp src/nlohmann/detail/input/parser.cpp
    src/nlohmann/detail/input/parser_types.h
    src/nlohmann/detail/input/position_t.hpp
    src/nlohmann/detail/iterators/internal_iterator.hpp
    src/nlohmann/detail/iterators/iteration_proxy.hpp
    src/nlohmann/detail/iterators/json_reverse_iterator.hpp
    src/nlohmann/detail/iterators/iter_impl.hpp
    src/nlohmann/detail/iterators/primitive_iterator.hpp
    src/nlohmann/detail/output/binary_writer.hpp src/nlohmann/detail/output/binary_writer.cpp
    src/nlohmann/detail/output/error_handler_t.h
    src/nlohmann/detail/output/output_adapters.hpp
    src/nlohmann/detail/output/serializer.hpp src/nlohmann/detail/output/serializer.cpp
    src/nlohmann/detail/json_ref.hpp
    src/nlohmann/detail/exceptions.hpp src/nlohmann/detail/exceptions.cpp
    src/nlohmann/detail/json_pointer.hpp src/nlohmann/detail/json_pointer.cpp
    src/nlohmann/detail/string_escape.hpp
    src/nlohmann/detail/string_concat.hpp
    src/nlohmann/detail/value_t.hpp src/nlohmann/detail/value_t.cpp
    src/nlohmann/detail/hash.hpp src/nlohmann/detail/hash.cpp
    src/nlohmann/ordered_map.hpp
    src/nlohmann/json.hpp src/nlohmann/json.cpp
    src/nlohmann/byte_container_with_subtype.hpp src/nlohmann/byte_container_with_subtype.cpp
    )
add_library(${PROJECT_NAME}::${NLOHMANN_JSON_TARGET_NAME} ALIAS ${NLOHMANN_JSON_TARGET_NAME})
target_compile_features(${NLOHMANN_JSON_TARGET_NAME} PUBLIC cxx_std_20)

target_include_directories(
    ${NLOHMANN_JSON_TARGET_NAME}
    ${NLOHMANN_JSON_SYSTEM_INCLUDE} PUBLIC
    $<BUILD_INTERFACE:${NLOHMANN_JSON_INCLUDE_BUILD_DIR}>
    $<INSTALL_INTERFACE:${NLOHMANN_JSON_INCLUDE_INSTALL_DIR}>
)

##
## TESTS
## create and configure the unit test target
##
if (JSON_BuildTests)
    include(CTest)
    enable_testing()
    add_subdirectory(tests)
endif()

##
## INSTALL
## install header files, generate and install cmake config files for find_package()
##
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    ${NLOHMANN_JSON_CMAKE_VERSION_CONFIG_FILE}
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)
configure_file(
    ${NLOHMANN_JSON_CMAKE_CONFIG_TEMPLATE}
    ${NLOHMANN_JSON_CMAKE_PROJECT_CONFIG_FILE}
    @ONLY
)

if(JSON_Install)
    install(
        DIRECTORY ${NLOHMANN_JSON_INCLUDE_BUILD_DIR}
        DESTINATION ${NLOHMANN_JSON_INCLUDE_INSTALL_DIR}
        FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h"
    )
    install(
        FILES ${NLOHMANN_JSON_CMAKE_PROJECT_CONFIG_FILE} ${NLOHMANN_JSON_CMAKE_VERSION_CONFIG_FILE}
        DESTINATION ${NLOHMANN_JSON_CONFIG_INSTALL_DIR}
    )
    export(
        TARGETS ${NLOHMANN_JSON_TARGET_NAME}
        NAMESPACE ${PROJECT_NAME}::
        FILE ${NLOHMANN_JSON_CMAKE_PROJECT_TARGETS_FILE}
    )
    install(
        TARGETS ${NLOHMANN_JSON_TARGET_NAME}
        EXPORT ${NLOHMANN_JSON_TARGETS_EXPORT_NAME}
        INCLUDES DESTINATION ${NLOHMANN_JSON_INCLUDE_INSTALL_DIR}
    )
    install(
        EXPORT ${NLOHMANN_JSON_TARGETS_EXPORT_NAME}
        NAMESPACE ${PROJECT_NAME}::
        DESTINATION ${NLOHMANN_JSON_CONFIG_INSTALL_DIR}
    )
endif()
