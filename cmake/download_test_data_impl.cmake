if(NOT EXISTS "${JSON_TEST_DATA_DIRECTORY}/README.md")
    execute_process(
        COMMAND "${JSON_GIT_EXECUTABLE}" clone -c advice.detachedHead=false
            --branch v3.1.0 --quiet --depth 1
            https://github.com/nlohmann/json_test_data.git "${JSON_TEST_DATA_DIRECTORY}"
        RESULT_VARIABLE result)
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "Downloading JSON test data failed: ${result}")
    endif()
endif()
