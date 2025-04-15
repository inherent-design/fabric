# Fabric Engine - CMake Build Configuration
cmake_minimum_required(VERSION 3.15)

# Project definition
project(fabric
    VERSION 0.1.0
    DESCRIPTION "Fabric Engine"
    LANGUAGES CXX
)

# Add cmake modules path
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/modules")

#------------------------------------------------------------------------------
# Build Configuration
#------------------------------------------------------------------------------

# C++ standard settings
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Output directories configuration
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

# Enable position independent code for libraries
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Platform-specific compiler flags
if(WIN32)
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Zi /Od")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /Zi /dynamicdeopt")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2")
else()
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -g -O3")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
endif()

# Platform-specific thread library linking
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

#------------------------------------------------------------------------------
# Generate Constants Header
#------------------------------------------------------------------------------
set(APP_NAME "Fabric")
set(APP_VERSION "${PROJECT_VERSION}")
set(APP_EXECUTABLE_NAME "Fabric")

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Constants.g.hh.in
    ${CMAKE_CURRENT_SOURCE_DIR}/include/fabric/core/Constants.g.hh
    @ONLY
)

#------------------------------------------------------------------------------
# Dependencies
#------------------------------------------------------------------------------
include(FetchContent)

# Google Test for testing
include(GoogleTest)

# SDL3 Configuration (only if needed)
option(FABRIC_USE_SDL "Use SDL for rendering and input" ON)
if(FABRIC_USE_SDL)
    FetchContent_Declare(
        SDL3
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG release-3.2.10
    )
    
    # SDL3 build options
    set(SDL_SHARED OFF CACHE BOOL "Build SDL shared libraries" FORCE)
    set(SDL_STATIC ON CACHE BOOL "Build SDL static libraries" FORCE)
    set(SDL_TEST OFF CACHE BOOL "Build SDL tests" FORCE)
    
    # Enable core SDL3 subsystems
    foreach(SUBSYSTEM IN ITEMS AUDIO VIDEO RENDER EVENTS JOYSTICK HIDAPI SENSOR THREADS TIMERS)
        set(SDL_${SUBSYSTEM} ON CACHE BOOL "Enable SDL ${SUBSYSTEM}" FORCE)
    endforeach()
    
    FetchContent_MakeAvailable(SDL3)
endif()

# WebView (only if needed)
option(FABRIC_USE_WEBVIEW "Use WebView for UI" ON)
if(FABRIC_USE_WEBVIEW)
    add_compile_definitions(FABRIC_USE_WEBVIEW)
endif()
if(FABRIC_USE_WEBVIEW)
    FetchContent_Declare(
        webview
        GIT_REPOSITORY https://github.com/webview/webview
        GIT_TAG 0.12.0
    )
    
    FetchContent_MakeAvailable(webview)
endif()

#------------------------------------------------------------------------------
# Source Files and Library Configuration
#------------------------------------------------------------------------------
# Core library components
set(FABRIC_CORE_SOURCE_FILES
    src/core/Component.cc
    src/core/Event.cc
    src/core/Lifecycle.cc
    src/core/Plugin.cc
    src/core/Command.cc
    src/core/Reactive.cc
    src/core/Resource.cc
    src/core/Spatial.cc
    src/core/Temporal.cc
    src/core/GraphResourceManager.cc
)

# Utils library components
file(GLOB FABRIC_UTILS_SOURCE_FILES src/utils/*.cc)

# Parser library components
file(GLOB FABRIC_PARSER_SOURCE_FILES src/parser/*.cc)

# UI library components (optional)
if(FABRIC_USE_WEBVIEW)
    file(GLOB FABRIC_UI_SOURCE_FILES src/ui/*.cc)
endif()

# Create the Fabric static library
add_library(FabricLib STATIC
    ${FABRIC_CORE_SOURCE_FILES}
    ${FABRIC_UTILS_SOURCE_FILES}
    ${FABRIC_PARSER_SOURCE_FILES}
    ${FABRIC_UI_SOURCE_FILES}
)

# Mark public API source files
set_source_files_properties(
    ${FABRIC_CORE_SOURCE_FILES}
    PROPERTIES
    COMPILE_DEFINITIONS "FABRIC_CORE_API"
)

# Include directories
target_include_directories(FabricLib 
    PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# Library dependencies
target_link_libraries(FabricLib 
    PUBLIC 
        Threads::Threads
)

# Add SDL3 if enabled
if(FABRIC_USE_SDL)
    target_link_libraries(FabricLib PUBLIC SDL3::SDL3-static)
endif()

# Add WebView if enabled
if(FABRIC_USE_WEBVIEW)
    target_link_libraries(FabricLib PUBLIC webview::core)
endif()

#------------------------------------------------------------------------------
# Main Executable
#------------------------------------------------------------------------------
# Create the main application executable if Fabric.cc exists
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/core/Fabric.cc")
    add_executable(Fabric src/core/Fabric.cc)
    target_link_libraries(Fabric PRIVATE FabricLib)
endif()

#------------------------------------------------------------------------------
# Testing Configuration
#------------------------------------------------------------------------------
option(FABRIC_BUILD_TESTS "Build Fabric tests" ON)

if(FABRIC_BUILD_TESTS)
    enable_testing()
    
    # Create test executables
    add_executable(UnitTests)
    add_executable(IntegrationTests)
    add_executable(E2ETests)
    
    # Link against GTest and FabricLib
    foreach(TEST_TARGET UnitTests IntegrationTests E2ETests)
        target_link_libraries(${TEST_TARGET} PRIVATE 
            gtest 
            gtest_main
            gmock
            gmock_main
            FabricLib
        )
    endforeach()
    
    # Add test directories
    add_subdirectory(tests)
endif()

#------------------------------------------------------------------------------
# Platform-Specific Configuration
#------------------------------------------------------------------------------
# macOS Configuration
if(APPLE)
    message(STATUS "Configuring for macOS")
    
    # Set minimum deployment target
    set(CMAKE_OSX_DEPLOYMENT_TARGET "14.0" CACHE STRING "Minimum macOS deployment version")
    
    # Build universal binary by default
    option(FABRIC_BUILD_UNIVERSAL "Build universal binaries for macOS" ON)
    if(FABRIC_BUILD_UNIVERSAL)
        set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Build architectures for macOS")
    endif()
    
    # Only add required frameworks if WebView/SDL are enabled
    if(FABRIC_USE_WEBVIEW)
        set(MACOS_UI_FRAMEWORKS
            Cocoa
            WebKit
        )
        
        foreach(FRAMEWORK ${MACOS_UI_FRAMEWORKS})
            target_link_libraries(FabricLib PUBLIC "-framework ${FRAMEWORK}")
        endforeach()
    endif()
    
    if(FABRIC_USE_SDL)
        set(MACOS_MEDIA_FRAMEWORKS
            CoreAudio
            AudioToolbox
            CoreHaptics
            CoreVideo
            Metal
            GameController
            IOKit
        )
        
        foreach(FRAMEWORK ${MACOS_MEDIA_FRAMEWORKS})
            target_link_libraries(FabricLib PUBLIC "-framework ${FRAMEWORK}")
        endforeach()
    endif()
endif()

# Linux Configuration
if(UNIX AND NOT APPLE)
    message(STATUS "Configuring for Linux")
    
    # Only search for dependencies if needed
    if(FABRIC_USE_WEBVIEW OR FABRIC_USE_SDL)
        find_package(PkgConfig REQUIRED)
        
        if(FABRIC_USE_WEBVIEW)
            pkg_check_modules(WEBKIT REQUIRED webkit2gtk-4.0)
            target_include_directories(FabricLib PUBLIC ${WEBKIT_INCLUDE_DIRS})
            target_link_libraries(FabricLib PUBLIC ${WEBKIT_LIBRARIES})
        endif()
        
        if(FABRIC_USE_SDL)
            pkg_check_modules(X11 REQUIRED x11 xrandr xext xi)
            target_include_directories(FabricLib PUBLIC ${X11_INCLUDE_DIRS})
            target_link_libraries(FabricLib PUBLIC ${X11_LIBRARIES})
        endif()
    endif()
endif()

# Windows Configuration
if(WIN32)
    message(STATUS "Configuring for Windows")
    
    # Set Windows version to Windows 7 or later
    add_compile_definitions(
        _WIN32_WINNT=0x0601
        WINVER=0x0601
        NTDDI_VERSION=0x06010000
    )
    
    # Configure as Windows GUI application when building the main executable
    if(TARGET Fabric)
        set_target_properties(Fabric PROPERTIES WIN32_EXECUTABLE TRUE)
    endif()
    
    # Only add Windows-specific libraries if needed
    if(FABRIC_USE_WEBVIEW OR FABRIC_USE_SDL)
        set(WINDOWS_LIBRARIES
            user32      # User Interface
            gdi32       # Graphics Device Interface
            ole32       # OLE 32-bit
            shell32     # Shell
            advapi32    # Advanced Windows 32 API
        )
        
        target_link_libraries(FabricLib PUBLIC ${WINDOWS_LIBRARIES})
    endif()
endif()

#------------------------------------------------------------------------------
# Installation Configuration
#------------------------------------------------------------------------------
# Set installation directories
include(GNUInstallDirs)

# Install library
install(TARGETS FabricLib
    EXPORT FabricTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Install main executable if it exists
if(TARGET Fabric)
    install(TARGETS Fabric
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif()

# Install headers
install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.hh"
)

# Generate and install package configuration
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/FabricConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(EXPORT FabricTargets
    FILE FabricTargets.cmake
    NAMESPACE Fabric::
    DESTINATION lib/cmake/Fabric
)

configure_file(cmake/FabricConfig.cmake.in "${CMAKE_CURRENT_BINARY_DIR}/FabricConfig.cmake" @ONLY)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/FabricConfig.cmake"
              "${CMAKE_CURRENT_BINARY_DIR}/FabricConfigVersion.cmake"
        DESTINATION lib/cmake/Fabric
)