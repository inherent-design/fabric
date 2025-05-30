# Fabric Engine Testing Guide

[← Back to Documentation Index](DOCUMENTATION.md)

## Table of Contents
- [Overview](#overview)
- [Testing Philosophy](#testing-philosophy)
- [Test Structure](#test-structure)
  - [Directory Organization](#directory-organization)
  - [Test Naming Conventions](#test-naming-conventions)
- [Unit Tests](#unit-tests)
  - [Examples](#examples)
- [Integration Tests](#integration-tests)
  - [Examples](#examples-1)
- [End-to-End Tests](#end-to-end-tests)
  - [Examples](#examples-2)
- [Mock Objects](#mock-objects)
  - [Test Fixtures](#test-fixtures)
- [Running Tests](#running-tests)
- [Adding New Tests](#adding-new-tests)
- [Continuous Integration](#continuous-integration)
- [Coverage Analysis](#coverage-analysis)

## Overview

Fabric's testing infrastructure is built using Google Test and Google Mock. The test suite is organized into three layers: unit tests, integration tests, and end-to-end tests. This multi-layered approach ensures that components work correctly in isolation and in combination.

## Testing Philosophy

Our testing strategy follows these core principles:

1. **Comprehensive Coverage**: Test all critical components and functionality.
2. **Test Isolation**: Unit tests should focus on a single component with minimal dependencies.
3. **Real-World Scenarios**: Integration and end-to-end tests should simulate actual use cases.
4. **Testability First**: Code should be designed with testability in mind.
5. **Continuous Testing**: Tests should be easy to run as part of the development workflow.
6. **Thread Safety Verification**: Critical thread-safe components must include concurrent access testing.
7. **Type Safety Assurance**: Tests must verify type safety guarantees across the API surface.

## Test Structure

### Directory Organization

Tests are organized into three main directories:

```
tests/
├── unit/                # Unit tests for individual components
│   ├── parser/          # Parser unit tests
│   │   └── ArgumentParserTest.cc
│   ├── ui/              # UI unit tests
│   │   └── WebViewTest.cc
│   └── utils/           # Utilities unit tests
│       ├── ErrorHandlingTest.cc
│       ├── LoggingTest.cc
│       └── UtilsTest.cc
├── integration/         # Integration tests between components
│   └── ParserLoggingIntegrationTest.cc
└── e2e/                 # End-to-end application tests
    └── FabricE2ETest.cc
```

### Test Naming Conventions

Test files follow a consistent naming pattern:

- Unit tests: `<ComponentName>Test.cc`
- Integration tests: `<Component1><Component2>IntegrationTest.cc`
- End-to-end tests: `<Feature>E2ETest.cc`

## Unit Tests

Unit tests focus on testing individual components in isolation. They typically involve:

- Testing a single class or function
- Mocking or stubbing dependencies
- Verifying specific behaviors and edge cases

### Examples

#### ArgumentParser Tests

```cpp
TEST_F(ArgumentParserTest, ParseSimpleFlag) {
    // Setup
    parser.addArgument("--test", "Test flag");

    // Prepare args
    const char* args[] = {"program", "--test"};
    int argc = sizeof(args) / sizeof(args[0]);

    // Execute
    parser.parse(argc, const_cast<char**>(args));

    // Verify
    ASSERT_TRUE(parser.hasArgument("--test"));
    auto token = parser.getArgument("--test");
    ASSERT_TRUE(token.has_value());
    ASSERT_TRUE(std::get<bool>(token.value().value));
}
```

#### Logging Tests

```cpp
TEST_F(LoggingTest, LogInfo) {
    // Execute
    Logger::logInfo("Info message");

    // Verify
    std::string output = capturedOutput.str();
    ASSERT_THAT(output, ::testing::HasSubstr("INFO"));
    ASSERT_THAT(output, ::testing::HasSubstr("Info message"));
}
```

#### Thread Safety Tests

```cpp
TEST_F(UtilsTest, TestGenerateUniqueIdThreadSafety) {
    const int numThreads = 10;
    const int idsPerThread = 100;
    std::unordered_set<std::string> generatedIds;
    std::mutex idsMutex;
    
    auto generateIdsTask = [&]() {
        std::vector<std::string> threadIds;
        threadIds.reserve(idsPerThread);
        
        for (int i = 0; i < idsPerThread; i++) {
            threadIds.push_back(Utils::generateUniqueId("thread_"));
        }
        
        // Add to the shared set with a lock
        std::lock_guard<std::mutex> lock(idsMutex);
        for (const auto& id : threadIds) {
            generatedIds.insert(id);
        }
    };
    
    // Launch threads
    std::vector<std::future<void>> futures;
    for (int i = 0; i < numThreads; i++) {
        futures.push_back(std::async(std::launch::async, generateIdsTask));
    }
    
    // Wait for all threads to complete
    for (auto& future : futures) {
        future.wait();
    }
    
    // Verify we have the expected number of unique IDs
    ASSERT_EQ(generatedIds.size(), numThreads * idsPerThread);
}
```

## Integration Tests

Integration tests verify the interaction between multiple components, ensuring they work together as expected. Common integration tests include:

- Communication between components
- Data flow across component boundaries
- Error handling and recovery across components

### Examples

#### Parser-Logging Integration Test

```cpp
TEST_F(ParserLoggingIntegrationTest, ValidParsingLogging) {
    // Setup and manually log
    capturedOutput.str("");
    Logger::logInfo("Testing parser with valid arguments");

    // Add argument and prepare args
    parser.addArgument("--verbose", "Verbose output");
    const char* args[] = {"program", "--verbose"};
    int argc = sizeof(args) / sizeof(args[0]);

    // Execute
    parser.parse(argc, const_cast<char**>(args));

    // Verify parser state
    ASSERT_TRUE(parser.isValid());
    ASSERT_TRUE(parser.hasArgument("--verbose"));

    // Verify log output
    std::string output = capturedOutput.str();
    ASSERT_THAT(output, ::testing::HasSubstr("INFO"));
    ASSERT_THAT(output, ::testing::HasSubstr("Testing parser with valid arguments"));
}
```

## End-to-End Tests

End-to-end tests verify the complete application functionality, ensuring that all components work together correctly in a real-world scenario. These tests:

- Simulate user interactions
- Test full application workflows
- Verify system-level behaviors

### Examples

#### Application Startup Test

```cpp
TEST_F(FabricE2ETest, DISABLED_VersionFlag) {
    // Execute Fabric with version flag
    std::string output = executeCommand(fabricPath + " --version");

    // Verify output
    ASSERT_THAT(output, ::testing::HasSubstr(Fabric::APP_NAME));
    ASSERT_THAT(output, ::testing::HasSubstr(Fabric::APP_VERSION));
}
```

## Mock Objects

For components with external dependencies, we use Google Mock to create mock objects. This allows testing components in isolation without requiring the actual dependencies.

### Test Fixtures

Most tests use a test fixture (a subclass of `::testing::Test`) to set up common state and utilities:

```cpp
class LoggingTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize logger for tests
        Logger::initialize();

        // Capture cout and cerr for verification
        originalCoutBuf = std::cout.rdbuf();
        originalCerrBuf = std::cerr.rdbuf();
        std::cout.rdbuf(capturedOutput.rdbuf());
        std::cerr.rdbuf(capturedErrOutput.rdbuf());
    }

    void TearDown() override {
        // Restore cout and cerr
        std::cout.rdbuf(originalCoutBuf);
        std::cerr.rdbuf(originalCerrBuf);
    }

    std::stringstream capturedOutput;
    std::stringstream capturedErrOutput;
    std::streambuf* originalCoutBuf;
    std::streambuf* originalCerrBuf;
};
```

#### Specialized Test Fixtures

For testing complex components like the resource management system that involve worker threads or concurrency, we use specialized fixtures that handle setup and cleanup of these resources:

```cpp
// A specialized test fixture for resource manager tests that need deterministic behavior
class ResourceDeterministicTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Register test factory
        ResourceFactory::registerType<TestResource>("test", TestResourceFactory::create);
        
        // Disable worker threads for deterministic testing
        ResourceManager::instance().disableWorkerThreadsForTesting();
        
        // Start with a large budget
        ResourceManager::instance().setMemoryBudget(std::numeric_limits<size_t>::max());
    }
    
    void TearDown() override {
        // Clean up resources
        ResourceManager& manager = ResourceManager::instance();
        
        // Restore default settings
        manager.setMemoryBudget(std::numeric_limits<size_t>::max());
        manager.enforceMemoryBudget();
        
        // Restart worker threads for other tests
        manager.restartWorkerThreadsAfterTesting();
    }
};
```

This specialized fixture handles:
- Registering necessary factories
- Disabling worker threads for deterministic testing
- Setting up test conditions
- Cleaning up resources after tests complete
- Restoring worker threads for subsequent tests
```

## Running Tests

Tests can be run using the following commands from the build directory:

```bash
# Run all unit tests
./bin/UnitTests

# Run all integration tests
./bin/IntegrationTests

# Run all end-to-end tests
./bin/E2ETests

# Run a specific test
./bin/UnitTests --gtest_filter=LoggingTest.LogInfo

# Run all tests matching a pattern
./bin/UnitTests --gtest_filter=ResourceDeterministicTest*

# Run all tests with verbose output
./bin/UnitTests --gtest_output=verbose

# Run tests with custom output format
./bin/UnitTests --gtest_color=yes --gtest_print_time=true
```

### Using Test Fixtures to Group Tests

When running tests, you can select all tests that use a specific fixture by filtering with the fixture name:

```bash
# Run all deterministic resource tests
./bin/UnitTests --gtest_filter="ResourceDeterministicTest*"

# Run a combination of test fixtures
./bin/UnitTests --gtest_filter="ResourceTest*:ResourceDeterministicTest*"

# Exclude specific tests
./bin/UnitTests --gtest_filter="ResourceTest*-ResourceTest.ResourceEviction"
```

## Adding New Tests

When adding new functionality, follow these steps to add tests:

1. Determine the appropriate test level (unit, integration, or end-to-end)
2. Create a new test file in the corresponding directory if needed
3. Write tests that cover both normal usage and edge cases
4. Ensure tests are independent and don't rely on global state
5. Run the tests to verify they pass

## Continuous Integration

Tests are automatically run as part of our CI/CD pipeline. Pull requests must pass all tests before they can be merged.

## Coverage Analysis

Code coverage analysis helps identify untested code paths. To generate a coverage report:

```bash
# Build with coverage enabled
cmake -DCMAKE_BUILD_TYPE=Debug -DENABLE_COVERAGE=ON ..
make

# Run tests
./bin/UnitTests
./bin/IntegrationTests
./bin/E2ETests

# Generate coverage report
make coverage-report
```

The coverage report will be available in the `coverage` directory.

---

[← Back to Documentation Index](DOCUMENTATION.md)
