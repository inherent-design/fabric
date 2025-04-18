# Fabric Build Guide

This guide explains how to efficiently build and test the Fabric project, with a focus on using modern build tools for the best development experience.

## Prerequisites

- CMake 3.14 or higher
- A C++20 compatible compiler:
  - GCC 10+
  - Clang 11+
  - MSVC 19.29+
- Ninja build system (recommended)
- CCache (optional but recommended)

## Basic Build Commands

### Standard Build

```bash
# Configure with default settings
cmake -B build

# Build the project
cmake --build build
```

### Optimized Build with Ninja

```bash
# Configure with Ninja generator
cmake -G "Ninja" -B build

# Build with multiple cores
cmake --build build
```

### Debug vs Release Builds

```bash
# Debug build (default)
cmake -G "Ninja" -B build -DCMAKE_BUILD_TYPE=Debug

# Release build
cmake -G "Ninja" -B build -DCMAKE_BUILD_TYPE=Release
```

## Improving Build Performance

### Using CCache

CCache dramatically speeds up rebuilds by caching previous compilations:

```bash
# Enable CCache in CMake
cmake -G "Ninja" -B build -DCMAKE_CXX_COMPILER_LAUNCHER=ccache

# Or, if using the Fabric-specific option
cmake -G "Ninja" -B build -DFABRIC_ENABLE_CCACHE=ON
```

### Parallel Builds

Ninja automatically determines the optimal number of cores to use, but you can manually specify:

```bash
# Build with a specific number of cores
cmake --build build -j 8
```

## Running Tests

### Running All Tests

```bash
# Build and run all tests
cmake --build build --target test

# Alternative using CTest directly
cd build && ctest
```

### Running Specific Tests

```bash
# Run a specific test by name
cd build && ctest -R ResourceHubTest

# Run tests with specific filter
./build/bin/UnitTests --gtest_filter=ResourceHub*

# Run a specific test with timeout protection to prevent hangs
./build/bin/UnitTests --gtest_filter=ResourceHub.LoadResource --gtest_timeout=5
```

## Build Options

The build system supports several options to customize your build:

| Option                    | Description                      | Default |
| ------------------------- | -------------------------------- | ------- |
| FABRIC_BUILD_TESTS        | Build the test suite             | ON      |
| FABRIC_ENABLE_CCACHE      | Use CCache if available          | ON      |
| FABRIC_USE_SANITIZERS     | Enable sanitizers in debug build | OFF     |
| FABRIC_DISABLE_EXCEPTIONS | Build without C++ exceptions     | OFF     |

Example:

```bash
cmake -G "Ninja" -B build \
    -DFABRIC_USE_SANITIZERS=ON \
    -DCMAKE_BUILD_TYPE=Debug
```

## Sanitizers

Sanitizers are helpful for catching memory and threading issues:

```bash
# Build with sanitizers enabled
cmake -G "Ninja" -B build -DFABRIC_USE_SANITIZERS=ON

# Run tests with sanitizer output
ASAN_OPTIONS=halt_on_error=0:symbolize=1 ./build/bin/UnitTests
```

## Common Issues

### ResourceHub Test Hangs

If tests hang, especially ResourceHub tests:

```bash
# Make sure to disable worker threads
./build/bin/UnitTests --gtest_filter=ResourceHub* --gtest_timeout=5
```

ResourceHub tests require special care - see [TESTING_BEST_PRACTICES.md](TESTING_BEST_PRACTICES.md) for details.

### Link Errors

If you encounter link errors, make sure all components are built:

```bash
# Clean build and rebuild all
rm -rf build && cmake -G "Ninja" -B build && cmake --build build
```

## Continuous Integration

The continuous integration setup uses:

1. CCache for faster builds
2. Ninja for parallel compilation
3. Comprehensive test timeout protection
4. Automatic sanitizer checks in debug mode

## Conclusion

For the best development experience with Fabric:

1. Use Ninja instead of Make
2. Enable CCache to speed up rebuilds
3. Run specific test groups to avoid unnecessary test execution
4. Use sanitizers in debug mode to catch issues early
