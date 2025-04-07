# Fabric Engine

Fabric Engine is a cross-platform application framework that provides a lightweight webview-based UI with native performance.

## Overview

Fabric Engine allows developers to build desktop applications using web technologies (HTML, CSS, JavaScript) while maintaining native performance and access to system capabilities. The engine uses a native webview component to render the UI and provides a bridge between JavaScript and native code.

## Project Structure

```
fabric/
├── cmake/                  # CMake configuration files
│   └── Constants.g.hh.in   # Template for generating constants
├── include/                # Public header files
├── src/                    # Source code
│   ├── ArgumentParser.cc   # Command-line argument parsing
│   ├── ArgumentParser.hh
│   ├── Constants.g.hh      # Generated constants
│   ├── ErrorHandling.cc    # Error handling utilities
│   ├── ErrorHandling.hh
│   ├── Fabric.cc           # Main application entry point
│   ├── Logging.cc          # Logging system
│   ├── Logging.hh
│   ├── SyntaxTree.cc       # Syntax tree for parsing
│   ├── SyntaxTree.hh
│   ├── Token.hh            # Token definitions for parser
│   ├── Utils.cc            # Utility functions
│   ├── Utils.hh
│   ├── WebView.cc          # Webview wrapper
│   └── WebView.hh
└── CMakeLists.txt          # Build configuration
```

## Architecture

Fabric Engine is built with a modular architecture:

1. **Core Engine** - The main application framework that handles initialization, event loop, and resource management
2. **WebView** - A wrapper around the native webview component that provides a consistent API across platforms
3. **ArgumentParser** - A robust command-line argument parser with support for flags, options, and validation
4. **Logging** - A simple logging system for debugging and error reporting
5. **Error Handling** - Utilities for consistent error handling throughout the application
6. **Utilities** - Common utility functions for string manipulation and other tasks

## Current Features

- **Cross-Platform Support**: Works on Windows, macOS, and Linux
- **Webview Integration**: Embeds a native webview component for UI rendering
- **Command-Line Interface**: Robust argument parsing with support for flags and options
- **Error Handling**: Comprehensive error handling and logging system

## Planned Features

- **HTML/CSS/JS UI Framework**: Complete UI framework for building interfaces using web technologies
- **Native API Bridge**: JavaScript to native code bridge for accessing system capabilities
- **Hot Reload**: Development mode with hot reload for rapid UI iteration
- **Developer Tools**: Integrated developer tools for debugging and profiling
- **Plugin System**: Extensible plugin architecture for adding custom functionality

## Proposed Features

- **State Management**: Built-in state management system with data binding
- **Component Library**: Pre-built UI component library with native look and feel
- **Offline Support**: Offline-first architecture with local storage and sync
- **Multi-Window Support**: Support for multiple application windows
- **Native Dialogs**: File pickers, alerts, and other native dialog integrations

## Quality of Life Improvements

- **Project Templates**: Starter templates for common application types
- **CLI Tool**: Command-line tool for project scaffolding and management
- **Documentation Generator**: Automatic documentation generation from code comments
- **Testing Framework**: Integrated testing framework for unit and UI tests
- **Performance Profiling**: Built-in performance measurement and optimization tools

## Trending Features

- **AI Integration**: APIs for integrating with AI services and local models
- **Real-Time Collaboration**: Built-in support for multi-user collaboration
- **WebAssembly Support**: Run compiled languages in the webview context
- **PWA Export**: Export applications as Progressive Web Apps
- **Responsive Design System**: Adaptive layout system for different screen sizes

## Building from Source

### Prerequisites

- CMake 3.16 or higher
- C++20 compatible compiler
- Platform-specific development tools:
  - Windows: Visual Studio with C++ workload
  - macOS: Xcode Command Line Tools
  - Linux: GCC/Clang and development packages

### Build Instructions

```bash
# Clone the repository
git clone https://github.com/yourusername/fabric.git
cd fabric

# Create build directory
mkdir build && cd build

# Configure and build
cmake ..
make

# Run the application
./bin/Fabric
```

## License

[MIT License](LICENSE)

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.
