# GoogleTest.cmake - Fetch and configure Google Test
include(FetchContent)

# Download and configure Google Test
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.16.0 # Use the latest release version
)

# Explicitly make Google Test available
FetchContent_MakeAvailable(googletest)

# Prevent installation of Google Test
set(INSTALL_GTEST OFF CACHE BOOL "Disable Google Test installation" FORCE)

# Add gtest and gmock include directories to the build
include_directories(
    ${googletest_SOURCE_DIR}/googletest/include
    ${googletest_SOURCE_DIR}/googlemock/include
)

# Function to add a test executable
function(add_fabric_test TEST_NAME TEST_SOURCES)
    # Create the test executable
    add_executable(${TEST_NAME} ${TEST_SOURCES})

    # Link with Google Test, GMock, and our library
    target_link_libraries(${TEST_NAME} PRIVATE
        GTest::gtest
        GTest::gtest_main
        GTest::gmock
        GTest::gmock_main
        FabricLib
    )

    # Add the test to CTest
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})

    # Set the test's working directory
    set_tests_properties(${TEST_NAME} PROPERTIES
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
endfunction()
