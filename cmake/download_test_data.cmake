if(JSON_TestDataDirectory)
    set(json_test_data_directory "${JSON_TestDataDirectory}")
    add_custom_target(download_test_data)
else()
    find_package(Git REQUIRED)
    set(json_test_data_directory "${CMAKE_BINARY_DIR}/json_test_data")
    add_custom_target(download_test_data
        COMMAND ${CMAKE_COMMAND}
            "-DJSON_TEST_DATA_DIRECTORY=${json_test_data_directory}"
            "-DJSON_GIT_EXECUTABLE=${GIT_EXECUTABLE}"
            -P "${CMAKE_CURRENT_LIST_DIR}/download_test_data_impl.cmake"
        COMMENT "Downloading JSON test data if needed"
        VERBATIM)
endif()
file(TO_CMAKE_PATH "${json_test_data_directory}" json_test_data_directory)
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/include")
file(WRITE "${CMAKE_BINARY_DIR}/include/test_data.hpp"
    "#define TEST_DATA_DIRECTORY \"${json_test_data_directory}\"\n")
