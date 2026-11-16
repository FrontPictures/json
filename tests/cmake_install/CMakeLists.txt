set(install_prefix "${CMAKE_CURRENT_BINARY_DIR}/prefix")
add_test(NAME cmake_install_package COMMAND ${CMAKE_COMMAND} --install ${PROJECT_BINARY_DIR}
    --prefix ${install_prefix} --config $<CONFIG>)
set_tests_properties(cmake_install_package PROPERTIES FIXTURES_SETUP installed_package)
add_test(NAME cmake_installed_configure COMMAND ${CMAKE_COMMAND}
    -G "${CMAKE_GENERATOR}" -A "${CMAKE_GENERATOR_PLATFORM}"
    -S ${PROJECT_SOURCE_DIR}/tests/cmake_import/project
    -B ${CMAKE_CURRENT_BINARY_DIR}/consumer
    -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
    -DCMAKE_PREFIX_PATH=${install_prefix})
set_tests_properties(cmake_installed_configure PROPERTIES
    FIXTURES_REQUIRED installed_package FIXTURES_SETUP installed_consumer)
add_test(NAME cmake_installed_build COMMAND ${CMAKE_COMMAND}
    --build ${CMAKE_CURRENT_BINARY_DIR}/consumer --config $<CONFIG>)
set_tests_properties(cmake_installed_build PROPERTIES FIXTURES_REQUIRED installed_consumer)
