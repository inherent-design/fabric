# Fabric Engine - CMake Build Configuration
# Minimum required CMake version
cmake_minimum_required(VERSION 4.0)

# Project definition
project(fabric
    VERSION 0.1.0
    DESCRIPTION "Fabric Engine"
    LANGUAGES CXX
)

# Include custom CMake modules
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/modules" ${CMAKE_MODULE_PATH})

# Include FetchContent for dependency management
include(FetchContent)

#------------------------------------------------------------------------------
# Build Configuration
#------------------------------------------------------------------------------
# C++ standard settings
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable testing
enable_testing()

# Output directories configuration
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

# Enable position independent code for static libraries
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Platform-specific compiler flags
if(WIN32)
    # Windows compiler flags
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Zi /Od")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /Zi /dynamicdeopt")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2")
else()
    # Unix-like compiler flags (Linux, macOS)
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -g -O3")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
endif()

#------------------------------------------------------------------------------
# Generate Constants Header
#------------------------------------------------------------------------------
# Set application constants
set(APP_NAME "Fabric")
set(APP_VERSION "${PROJECT_VERSION}")
set(APP_EXECUTABLE_NAME "Fabric")

# Configure Constants.g.h file
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Constants.g.hh.in
    ${CMAKE_CURRENT_SOURCE_DIR}/include/fabric/core/Constants.g.hh
    @ONLY
)

#------------------------------------------------------------------------------
# Dependencies
#------------------------------------------------------------------------------
# SDL3 Configuration
FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-3.2.10
)

# SDL3 build options
set(SDL_OPTIONS
    SDL_SHARED=OFF
    SDL_STATIC=ON
    SDL_TEST=OFF
    # Enable all SDL3 subsystems
    SDL_AUDIO=ON
    SDL_VIDEO=ON
    SDL_RENDER=ON
    SDL_EVENTS=ON
    SDL_JOYSTICK=ON
    SDL_HAPTIC=ON
    SDL_HIDAPI=ON
    SDL_SENSOR=ON
    SDL_POWER=ON
    SDL_THREADS=ON
    SDL_TIMERS=ON
    SDL_FILE=ON
    SDL_CPUINFO=ON
    SDL_FILESYSTEM=ON
    SDL_LOCALE=ON
    SDL_MISC=ON
)

# Apply SDL3 options
foreach(OPTION ${SDL_OPTIONS})
    string(REPLACE "=" ";" OPTION_LIST ${OPTION})
    list(GET OPTION_LIST 0 OPTION_NAME)
    list(GET OPTION_LIST 1 OPTION_VALUE)
    set(${OPTION_NAME} ${OPTION_VALUE} CACHE BOOL "SDL3 option" FORCE)
endforeach()

# Webview Configuration
FetchContent_Declare(
    webview
    GIT_REPOSITORY https://github.com/webview/webview
    GIT_TAG 0.12.0
)

# Make dependencies available
FetchContent_MakeAvailable(SDL3 webview)

# Include Google Test for testing
include(GoogleTest)

#------------------------------------------------------------------------------
# Source Files and Library Configuration
#------------------------------------------------------------------------------
# Include project headers
include_directories(${PROJECT_SOURCE_DIR}/include)

# Define the source files for each module
file(GLOB_RECURSE CORE_SRC CONFIGURE_DEPENDS src/core/*.cc)
file(GLOB_RECURSE UI_SRC CONFIGURE_DEPENDS src/ui/*.cc)
file(GLOB_RECURSE UTILS_SRC CONFIGURE_DEPENDS src/utils/*.cc)
file(GLOB_RECURSE PARSER_SRC CONFIGURE_DEPENDS src/parser/*.cc)

# Create a static library for the core engine
add_library(FabricLib STATIC
    ${CORE_SRC}
    ${UI_SRC}
    ${UTILS_SRC}
    ${PARSER_SRC}
)

# Set include directories for the library
target_include_directories(FabricLib PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)

# Link dependencies to the library
target_link_libraries(FabricLib PUBLIC
    SDL3::SDL3-static
    webview::core
)

#------------------------------------------------------------------------------
# Main Executable
#------------------------------------------------------------------------------
# Define the main application
add_executable(Fabric src/core/Fabric.cc)

# Link with the core library
target_link_libraries(Fabric PRIVATE FabricLib)

#------------------------------------------------------------------------------
# Tests Configuration
#------------------------------------------------------------------------------
# Find all test files
file(GLOB_RECURSE UNIT_TESTS CONFIGURE_DEPENDS tests/unit/*.cc)
file(GLOB_RECURSE INTEGRATION_TESTS CONFIGURE_DEPENDS tests/integration/*.cc)
file(GLOB_RECURSE E2E_TESTS CONFIGURE_DEPENDS tests/e2e/*.cc)

# Create a unit test executable
if(UNIT_TESTS)
    add_fabric_test(UnitTests "${UNIT_TESTS}")
endif()

# Create an integration test executable
if(INTEGRATION_TESTS)
    add_fabric_test(IntegrationTests "${INTEGRATION_TESTS}")
endif()

# Create an end-to-end test executable
if(E2E_TESTS)
    add_fabric_test(E2ETests "${E2E_TESTS}")
endif()

#------------------------------------------------------------------------------
# Platform-Specific Configuration
#------------------------------------------------------------------------------
# macOS Configuration
if(APPLE)
    message(STATUS "Configuring for macOS")

    # Set minimum deployment target for macOS 14+
    set(CMAKE_OSX_DEPLOYMENT_TARGET "14.0" CACHE STRING "Minimum macOS deployment version")

    # Build universal binary for both Intel and Apple Silicon
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Build architectures for macOS")

    # Required macOS frameworks
    set(MACOS_FRAMEWORKS
        Cocoa
        WebKit
        CoreAudio
        AudioToolbox
        CoreHaptics
        GameController
        CoreVideo
        ForceFeedback
        Metal
        IOKit
        Carbon
    )

    # Link with macOS frameworks
    foreach(FRAMEWORK ${MACOS_FRAMEWORKS})
        target_link_libraries(FabricLib PUBLIC "-framework ${FRAMEWORK}")
    endforeach()
endif()

# iOS Configuration
if(IOS)
    message(STATUS "Configuring for iOS")

    # Set minimum deployment target for iOS 17+
    set(CMAKE_OSX_DEPLOYMENT_TARGET "17.0" CACHE STRING "Minimum iOS deployment version")

    # Required iOS frameworks
    set(IOS_FRAMEWORKS
        UIKit
        Foundation
        CoreGraphics
        QuartzCore
        CoreAudio
        AudioToolbox
        CoreHaptics
        GameController
        Metal
        AVFoundation
    )

    # Link with iOS frameworks
    foreach(FRAMEWORK ${IOS_FRAMEWORKS})
        target_link_libraries(FabricLib PUBLIC "-framework ${FRAMEWORK}")
    endforeach()
endif()

# Linux Configuration
if(UNIX AND NOT APPLE)
    message(STATUS "Configuring for Linux (kernel 6.6+)")

    # Document minimum kernel version (informational only)
    set(LINUX_MIN_KERNEL "6.6" CACHE STRING "Minimum Linux kernel version")

    # Find required Linux packages
    find_package(PkgConfig REQUIRED)

    # Define required Linux dependencies
    set(LINUX_DEPENDENCIES
        webkit2gtk-4.0
        alsa
        libpulse
        wayland-client
        wayland-cursor
        wayland-egl
        wayland-protocols
        x11
        xext
        xrandr
        xrender
        xi
    )

    # Find and link Linux dependencies
    foreach(DEP ${LINUX_DEPENDENCIES})
        pkg_check_modules(${DEP} REQUIRED ${DEP})
        target_include_directories(FabricLib PUBLIC ${${DEP}_INCLUDE_DIRS})
        target_link_libraries(FabricLib PUBLIC ${${DEP}_LIBRARIES})
    endforeach()
endif()

# Windows Configuration
if(WIN32)
    message(STATUS "Configuring for Windows 7+")

    # Set Windows version to Windows 7 or later
    add_compile_definitions(
        _WIN32_WINNT=0x0601
        WINVER=0x0601
        NTDDI_VERSION=0x06010000
    )

    # Configure as Windows GUI application - Fix the subsystem conflict
    set_target_properties(Fabric PROPERTIES
        WIN32_EXECUTABLE TRUE
    )

    # Required Windows libraries
    set(WINDOWS_LIBRARIES
        advapi32    # Advanced Windows 32 API
        ole32       # OLE 32-bit
        shell32     # Shell
        shlwapi     # Shell Light-weight API
        user32      # User Interface
        version     # Version checking
        winmm       # Windows Multimedia
        imm32       # Input Method Manager
        setupapi    # Setup API
        gdi32       # Graphics Device Interface
        oleaut32    # OLE Automation
        uuid        # UUID generation
    )

    # Link with Windows libraries
    target_link_libraries(FabricLib PUBLIC ${WINDOWS_LIBRARIES})
endif()
