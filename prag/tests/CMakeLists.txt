	# ============================================================================
# Testing Setup
# ============================================================================


if(NOT MSVC)

    # Enable testing
    enable_testing()

    # Find GTest package (Google Test)
    find_package(GTest REQUIRED)

    # Include directories for tests
    set(TEST_INCLUDE_DIRECTORIES
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/include
        src/input
        src/output
        src/ast
    )

    # Link libraries for tests
    set(TEST_LINK_LIBRARIES
        GTest::gtest
        GTest::gtest_main
        ast
        inputs
        outputs
    )

    # Define a function to simplify adding test executables
    function(add_test_executable target_name source_file)
        add_executable(${target_name} ${source_file} test_util.cpp)
        target_include_directories(${target_name} PRIVATE ${TEST_INCLUDE_DIRECTORIES})
        target_link_libraries(${target_name} ${TEST_LINK_LIBRARIES})
    endfunction()

    add_test_executable(auto auto.cpp)


    

    # Set the working directory for the tests
    set(TEST_WORKING_DIR ${CMAKE_BINARY_DIR}/tests)

  # Copy test input files to the build directory for use in tests
    file(COPY ${CMAKE_SOURCE_DIR}/tests/h/inputs
         DESTINATION ${CMAKE_BINARY_DIR}/tests)

    file(COPY ${CMAKE_SOURCE_DIR}/tests/proto/inputs
         DESTINATION ${CMAKE_BINARY_DIR}/tests)



    # Copy the inputs directory to the correct path inside the build directory
    file(COPY ${CMAKE_SOURCE_DIR}/tests/h/inputs
       DESTINATION ${CMAKE_BINARY_DIR}/tests/cpp)
       
    # Copy the inputs directory to the correct path inside the build directory
    file(COPY ${CMAKE_SOURCE_DIR}/tests/proto/inputs
       DESTINATION ${CMAKE_BINARY_DIR}/tests/proto)
     
    # Custom target to run tests with verbose output
    add_custom_target(run_tests
        COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test_roundtrip --gtest_color=yes
        DEPENDS test_roundtrip
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )


endif()
