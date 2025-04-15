# Fabric Engine Build Guide

[← Back to Documentation Index](DOCUMENTATION.md)

## Table of Contents
- [Prerequisites](#prerequisites)
  - [Required Tools](#required-tools)
  - [Optional Tools](#optional-tools)
- [Platform-Specific Requirements](#platform-specific-requirements)
  - [macOS](#macos)
  - [Windows](#windows)
  - [Linux](#linux)
- [Building Fabric](#building-fabric)
  - [Basic Build](#basic-build)
  - [Build Options](#build-options)
  - [Using Ninja for Faster Builds](#using-ninja-for-faster-builds)
  - [Using CCache for Faster Compilation](#using-ccache-for-faster-compilation)
  - [Combining Ninja and CCache](#combining-ninja-and-ccache)
  - [Building on macOS](#building-on-macos)
  - [Building on Windows](#building-on-windows)
  - [Building on Linux](#building-on-linux)
- [Running Tests](#running-tests)
- [Running the Application](#running-the-application)
- [Common Build Issues](#common-build-issues)
- [Dependencies](#dependencies)
- [Custom Builds](#custom-builds)
- [Continuous Integration](#continuous-integration)
- [Additional Resources](#additional-resources)

## Prerequisites

To build Fabric Engine, you need the following tools and libraries:

### Required Tools

- **CMake** (version 4.0 or higher)
- **C++20 Compliant Compiler**:
  - GCC 10+ (Linux)
  - Clang 13+ (macOS, Linux)
  - MSVC 19.29+ (Windows)
- **Git** (for fetching dependencies)

### Optional Tools

- **Ninja** (for faster builds)
- **CCache** (for caching compilations)

## Platform-Specific Requirements

### macOS

- macOS 14.0 or later
- Xcode Command Line Tools (run `xcode-select --install`)
- Required frameworks (usually included with macOS):
  - Cocoa
  - WebKit
  - CoreAudio
  - AudioToolbox
  - CoreHaptics
  - CoreVideo
  - Metal
  - GameController
  - IOKit

### Windows

- Windows 7 or later
- Visual Studio 2019 or later with C++ Desktop Development workload
- Windows SDK 10.0.19041.0 or later
- Required libraries:
  - user32
  - gdi32
  - ole32
  - shell32
  - advapi32

### Linux

- Linux Kernel 6.6 or later
- Required packages:
  ```bash
  # Ubuntu/Debian
  sudo apt install build-essential cmake libwebkit2gtk-4.0-dev \
    libx11-dev libxrandr-dev libxext-dev libxi-dev

  # Fedora
  sudo dnf install gcc-c++ cmake webkit2gtk4.0-devel \
    libX11-devel libXrandr-devel libXext-devel libXi-devel

  # Arch Linux
  sudo pacman -S base-devel cmake webkit2gtk \
    libx11 libxrandr libxext libxi
  ```

## Building Fabric

### Basic Build

1. Clone the repository:
   ```bash
   git clone https://github.com/yourusername/fabric.git
   cd fabric
   ```

2. Create a build directory:
   ```bash
   mkdir -p build
   cd build
   ```

3. Configure the project:
   ```bash
   cmake .. -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug
   ```

4. Build the project:
   ```bash
   make
   ```

### Build Options

The following options can be passed to CMake:

- `-DCMAKE_BUILD_TYPE=<type>`: Set the build type (Debug, Release, RelWithDebInfo, MinSizeRel)
- `-DFABRIC_BUILD_TESTS=ON/OFF`: Enable or disable tests (default: ON)
- `-DCMAKE_INSTALL_PREFIX=<path>`: Set the installation path

### Using Ninja for Faster Builds

[Ninja](https://ninja-build.org/) is a build system designed for speed, especially for incremental builds. It's significantly faster than Make or MSBuild.

```bash
# Configure with Ninja
cmake .. -G "Ninja" -DCMAKE_BUILD_TYPE=Debug

# Build with Ninja
ninja
```

### Using CCache for Faster Compilation

[CCache](https://ccache.dev/) is a compiler cache that speeds up recompilation by caching previous compilations and detecting when the same compilation is being done again. This is particularly useful during development when you're making small changes and recompiling frequently.

```bash
# Configure CMake to use ccache
cmake .. -DCMAKE_CXX_COMPILER_LAUNCHER=ccache -DCMAKE_C_COMPILER_LAUNCHER=ccache -DCMAKE_BUILD_TYPE=Debug

# Build as normal
make
```

### Combining Ninja and CCache

For maximum build performance, you can combine Ninja and CCache:

```bash
# Configure with both Ninja and CCache
cmake .. -G "Ninja" -DCMAKE_CXX_COMPILER_LAUNCHER=ccache -DCMAKE_C_COMPILER_LAUNCHER=ccache -DCMAKE_BUILD_TYPE=Debug

# Build with Ninja
ninja
```

This combination provides both the parallel build speed of Ninja and the compilation caching of CCache, significantly reducing build times during development.

### Building on macOS

```bash
# Use Xcode's clang
cmake .. -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug
make
```

### Building on Windows

```cmd
# Using Visual Studio generator
cmake .. -G "Visual Studio 16 2019" -A x64
cmake --build . --config Debug

# Using Ninja (if available)
cmake .. -G "Ninja" -DCMAKE_BUILD_TYPE=Debug
ninja
```

### Building on Linux

```bash
cmake .. -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug
make
```

## Running Tests

After building the project, you can run the tests:

```bash
# Run all unit tests
./bin/UnitTests

# Run all integration tests
./bin/IntegrationTests

# Run all end-to-end tests
./bin/E2ETests
```

## Running the Application

After building, you can run Fabric:

```bash
# Run Fabric
./bin/Fabric

# Display help
./bin/Fabric --help

# Display version
./bin/Fabric --version
```

## Common Build Issues

### Missing Dependencies

If CMake reports missing dependencies, ensure all required packages are installed for your platform.

### Compiler Version

If you see errors related to C++20 features, ensure your compiler supports C++20:

```bash
# Check GCC version
gcc --version

# Check Clang version
clang --version
```

### WebView Issues

If you encounter issues with the WebView component:

- On Linux, ensure webkit2gtk-4.0 is installed
- On macOS, ensure you have the latest Xcode Command Line Tools
- On Windows, ensure you have the latest Windows SDK

## Dependencies

Fabric fetches the following dependencies automatically during the build process:

- **SDL3**: For cross-platform window management
- **WebView**: For embedding web browser components
- **Google Test**: For unit and integration testing

## Core Components

Fabric includes several core components with specific threading and synchronization requirements:

- **CoordinatedGraph**: Intent-based thread-safe graph implementation (see [CONCURRENCY.md](CONCURRENCY.md))
- **ResourceHub**: Centralized resource management using the CoordinatedGraph
- **Event System**: Thread-safe event dispatch mechanism

## Custom Builds

### Cross-Compilation

For cross-compilation, set the appropriate CMake toolchain file:

```bash
cmake .. -DCMAKE_TOOLCHAIN_FILE=<path_to_toolchain_file>
```

### Building for Multiple Architectures on macOS

```bash
cmake .. -DCMAKE_OSX_ARCHITECTURES="x86_64;arm64"
```

### Building a Static Library

By default, Fabric builds a static library. No special configuration is needed.

### Building a Shared Library

Currently, Fabric does not support building as a shared library.

## Continuous Integration

Fabric uses GitHub Actions for CI. Each pull request is automatically built and tested on all supported platforms.

## Additional Resources

- [CMake Documentation](https://cmake.org/documentation/)
- [Google Test Documentation](https://google.github.io/googletest/)
- [SDL3 Documentation](https://wiki.libsdl.org/SDL3/)
- [WebView Documentation](https://github.com/webview/webview)

---

[← Back to Documentation Index](DOCUMENTATION.md)
