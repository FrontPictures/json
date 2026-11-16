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

set(NLOHMANN_JSON_INCLUDE_BUILD_DIR "${PROJECT_SOURCE_DIR}/include/")

if (JSON_SystemInclude)
    set(NLOHMANN_JSON_SYSTEM_INCLUDE "SYSTEM")
endif()

##
## TARGET
## create target and add include path
##
add_library(${NLOHMANN_JSON_TARGET_NAME} STATIC
    include/nlohmann/detail/conversions/diyfp.h include/nlohmann/detail/conversions/diyfp.cpp
    include/nlohmann/detail/conversions/grisu2.h include/nlohmann/detail/conversions/grisu2.cpp
    include/nlohmann/detail/conversions/to_chars.hpp include/nlohmann/detail/conversions/to_chars.cpp
    include/nlohmann/detail/input/binary_reader.hpp include/nlohmann/detail/input/binary_reader.cpp
    include/nlohmann/detail/input/input_adapters.hpp include/nlohmann/detail/input/input_adapters.cpp
    include/nlohmann/detail/input/input_format_t.h include/nlohmann/detail/input/input_format_t.cpp
    include/nlohmann/detail/input/json_sax.hpp include/nlohmann/detail/input/json_sax.cpp
    include/nlohmann/detail/input/lexer.hpp include/nlohmann/detail/input/lexer.cpp
    include/nlohmann/detail/input/parser.hpp include/nlohmann/detail/input/parser.cpp
    include/nlohmann/detail/input/parser_types.h
    include/nlohmann/detail/input/position_t.hpp
    include/nlohmann/detail/iterators/internal_iterator.hpp
    include/nlohmann/detail/iterators/iteration_proxy.hpp
    include/nlohmann/detail/iterators/json_reverse_iterator.hpp
    include/nlohmann/detail/iterators/iter_impl.hpp
    include/nlohmann/detail/iterators/primitive_iterator.hpp
    include/nlohmann/detail/output/binary_writer.hpp include/nlohmann/detail/output/binary_writer.cpp
    include/nlohmann/detail/output/error_handler_t.h
    include/nlohmann/detail/output/output_adapters.hpp
    include/nlohmann/detail/output/serializer.hpp include/nlohmann/detail/output/serializer.cpp
    include/nlohmann/detail/json_ref.hpp
    include/nlohmann/detail/exceptions.hpp include/nlohmann/detail/exceptions.cpp
    include/nlohmann/detail/json_pointer.hpp include/nlohmann/detail/json_pointer.cpp
    include/nlohmann/detail/string_escape.hpp
    include/nlohmann/detail/string_concat.hpp
    include/nlohmann/detail/value_t.hpp include/nlohmann/detail/value_t.cpp
    include/nlohmann/detail/hash.hpp include/nlohmann/detail/hash.cpp
    include/nlohmann/ordered_map.hpp
    include/nlohmann/json.hpp include/nlohmann/json.cpp
    include/nlohmann/byte_container_with_subtype.hpp include/nlohmann/byte_container_with_subtype.cpp
    )
add_library(${PROJECT_NAME}::${NLOHMANN_JSON_TARGET_NAME} ALIAS ${NLOHMANN_JSON_TARGET_NAME})
target_compile_features(${NLOHMANN_JSON_TARGET_NAME} PUBLIC cxx_std_20)

target_include_directories(
    ${NLOHMANN_JSON_TARGET_NAME}
    ${NLOHMANN_JSON_SYSTEM_INCLUDE} PUBLIC
    $<BUILD_INTERFACE:${NLOHMANN_JSON_INCLUDE_BUILD_DIR}>
    $<INSTALL_INTERFACE:${NLOHMANN_JSON_INCLUDE_INSTALL_DIR}>
)

## add debug view definition file for msvc (natvis)
if (MSVC)
    set(NLOHMANN_ADD_NATVIS TRUE)
    set(NLOHMANN_NATVIS_FILE "nlohmann_json.natvis")
    target_sources(
        ${NLOHMANN_JSON_TARGET_NAME}
        PUBLIC
            $<INSTALL_INTERFACE:${NLOHMANN_NATVIS_FILE}>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/${NLOHMANN_NATVIS_FILE}>
    )
endif()

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
    if (NLOHMANN_ADD_NATVIS)
        install(
            FILES ${NLOHMANN_NATVIS_FILE}
            DESTINATION .
    )
    endif()
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
