# Fabric Engine Architecture

## Overview

Fabric is a modern C++20 cross-platform framework for building applications with an embedded web UI. It combines native C++ performance with the flexibility of HTML/CSS/JavaScript for the user interface.

## Project Structure

The project follows a modular architecture organized into logical components:

```
fabric/
├── CMakeLists.txt          # Main build configuration
├── README.md               # Project overview
├── cmake/                  # CMake modules and configuration
│   ├── Constants.g.hh.in   # Template for constants generation
│   └── modules/            # Custom CMake modules
│       └── GoogleTest.cmake # Google Test integration
├── docs/                   # Documentation
│   └── ARCHITECTURE.md     # This document
├── include/                # Public API headers
│   └── fabric/             # Main include directory
│       ├── core/           # Core framework components
│       │   └── Constants.g.hh # Generated constants
│       ├── parser/         # Command line and syntax parsing
│       │   ├── ArgumentParser.hh
│       │   ├── SyntaxTree.hh
│       │   └── Token.hh
│       ├── ui/             # User interface components
│       │   └── WebView.hh
│       └── utils/          # Utility functions and classes
│           ├── ErrorHandling.hh
│           ├── Logging.hh
│           └── Utils.hh
├── src/                    # Implementation files
│   ├── core/               # Core implementation
│   │   └── Fabric.cc       # Application entry point
│   ├── parser/             # Parser implementations
│   │   ├── ArgumentParser.cc
│   │   └── SyntaxTree.cc
│   ├── ui/                 # UI implementations
│   │   └── WebView.cc
│   └── utils/              # Utilities implementations
│       ├── ErrorHandling.cc
│       ├── Logging.cc
│       └── Utils.cc
└── tests/                  # Test suite
    ├── e2e/                # End-to-end tests
    │   └── FabricE2ETest.cc
    ├── integration/        # Integration tests
    │   └── ParserLoggingIntegrationTest.cc
    └── unit/               # Unit tests
        ├── parser/         # Parser unit tests
        │   └── ArgumentParserTest.cc
        ├── ui/             # UI unit tests
        │   └── WebViewTest.cc
        └── utils/          # Utilities unit tests
            ├── ErrorHandlingTest.cc
            ├── LoggingTest.cc
            └── UtilsTest.cc
```

## Component Design

### Core Components

The core components provide the application framework and entry point:

- **Fabric.cc**: Application entry point that initializes subsystems and handles the main event loop.
- **Constants.g.hh**: Generated constants used throughout the application.

### Parser Components

The parser system handles command-line arguments and syntax parsing:

- **ArgumentParser**: Processes command-line arguments using a builder pattern for configuration.
- **SyntaxTree**: Handles parsing of structured data.
- **Token**: Defines the token system for lexical analysis with an extensive type system.

### UI Components

The UI system provides a web-based user interface:

- **WebView**: Embeds a web browser view for rendering HTML/CSS/JavaScript interfaces.

### Utility Components

Utility components provide common functionality used throughout the application:

- **Logging**: Configurable logging system with multiple severity levels.
- **ErrorHandling**: Exception handling and error reporting.
- **Utils**: General utility functions for string manipulation, file operations, etc.

## Build System

Fabric uses CMake (4.0+) as its build system with the following features:

- **Static Library**: Core functionality is built as a static library (`FabricLib`)
- **Executable**: Main application built on top of the library
- **Dependencies**:
  - SDL3: For cross-platform window management, input handling, and multimedia
  - Webview: For embedding a web browser component
  - Google Test: For unit and integration testing

## Testing Architecture

The testing system is organized into three layers:

### Unit Tests

Unit tests focus on testing individual components in isolation:

- **ArgumentParserTest**: Tests CLI argument parsing functionality
- **WebViewTest**: Tests UI webview functionality
- **LoggingTest**: Tests logging system
- **ErrorHandlingTest**: Tests error handling
- **UtilsTest**: Tests utility functions

### Integration Tests

Integration tests verify the interaction between multiple components:

- **ParserLoggingIntegrationTest**: Tests integration between the ArgumentParser and Logger

### End-to-End Tests

End-to-end tests verify the complete application functionality:

- **FabricE2ETest**: Tests full application execution with command-line parameters

## Testing Execution

Tests can be run using the following commands from the build directory:

```bash
# Run all unit tests
./bin/UnitTests

# Run all integration tests
./bin/IntegrationTests

# Run all end-to-end tests
./bin/E2ETests
```

## Platform Support

Fabric is designed to be cross-platform with support for:

- **macOS**: Version 14.0+ (Universal binary for Intel and Apple Silicon)
- **Windows**: Windows 7+
- **Linux**: Kernel 6.6+

## Dependencies

- **SDL3**: Core platform abstraction for windowing, input, and more
- **Webview**: Library for embedding web browser views
- **Google Test**: Testing framework for unit and integration tests

## Future Expansion

The modular design allows for easy expansion in these areas:

1. Additional UI components
2. More parser capabilities
3. Enhanced platform-specific features
4. Extended utility libraries

## Initialization Flow

1. **Logger Initialization**: First subsystem to initialize
2. **Command Line Parsing**: Process startup arguments
3. **WebView Initialization**: Set up the UI component
4. **Main Event Loop**: Process events and handle user interaction
