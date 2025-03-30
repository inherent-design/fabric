cmake_minimum_required(VERSION 3.16)
project(fabric LANGUAGES CXX)

include(FetchContent)

# C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Output dirs
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

# Fetch webview
FetchContent_Declare(
    webview
    GIT_REPOSITORY https://github.com/webview/webview
    GIT_TAG 0.12.0
)
FetchContent_MakeAvailable(webview)

# Include your own headers
include_directories(${CMAKE_SOURCE_DIR}/include)

# Source files
file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS src/*.cpp)

# Create executable
add_executable(fabric ${SRC_FILES})

# Link with webview
target_link_libraries(fabric PRIVATE webview::core)

# macOS platform-specific settings
if(APPLE)
    message(STATUS "Configuring for macOS")

    # Link with macOS frameworks
    target_link_libraries(fabric
        PRIVATE
            "-framework Cocoa"
            "-framework WebKit")
endif()

# Linux-specific settings
if(UNIX AND NOT APPLE)
    message(STATUS "Configuring for Linux")

    find_package(PkgConfig REQUIRED)
    pkg_check_modules(WEBKIT REQUIRED webkit2gtk-4.0)

    target_include_directories(fabric PRIVATE ${WEBKIT_INCLUDE_DIRS})
    target_link_libraries(fabric PRIVATE ${WEBKIT_LIBRARIES})
endif()

# Windows-specific flags
if(WIN32)
    message(STATUS "Configuring for Windows")

    target_link_libraries(fabric
        PRIVATE
            advapi32
            ole32
            shell32
            shlwapi
            user32
            version)
endif()
