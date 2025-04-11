# Contributing to Fabric Engine

> *"To contribute meaningfully, first understand that your perspective shapes what you perceive as problems and solutions. The most valuable insights often come from shifting to a viewpoint where the problem itself is transformed."*

## Getting Started

Thank you for your interest in contributing to Fabric Engine! This document provides guidelines and information to help you get started.

### Prerequisites

Before you begin contributing, ensure you have the following:

- CMake 4.0 or higher
- C++20 compatible compiler
- Platform-specific development tools (Visual Studio, Xcode Command Line Tools, or GCC/Clang)
- Understanding of the Fabric Engine's perspective-fluid architecture

### Development Environment Setup

1. Fork the repository on GitHub
2. Clone your fork locally:
   ```bash
   git clone https://github.com/yourusername/fabric.git
   cd fabric
   ```
3. Set up your build environment:
   ```bash
   mkdir build && cd build
   cmake .. -DCMAKE_BUILD_TYPE=Debug
   make
   ```
4. Run the tests to ensure everything is working:
   ```bash
   make test
   ```

## Project Structure

The Fabric Engine uses a modular architecture designed for flexibility and extensibility:

- **`/src`** - Source code
  - **`/src/core`** - Core framework components (Quantum, Scope, Perspective)
  - **`/src/util`** - Utilities and helpers
  - **`/src/physics`** - Scale-dependent physics systems
  - **`/src/render`** - Visualization and rendering
  - **`/src/webview`** - Web integration components
- **`/include`** - Public headers
- **`/tests`** - Unit and integration tests
- **`/examples`** - Example applications
- **`/docs`** - Documentation
- **`/cmake`** - CMake configuration files

## Coding Guidelines

### C++ Style Guidelines

- Follow modern C++20 practices
- Use STL containers and algorithms where appropriate
- Prefer strong typing and avoid raw pointers when possible
- Use `std::unique_ptr` and `std::shared_ptr` for ownership management
- Follow the [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines)

### Naming Conventions

- Classes, structs, and enums: `PascalCase`
- Functions and methods: `camelCase`
- Variables: `snake_case`
- Member variables: `snake_case_` with trailing underscore
- Constants: `kConstantName`
- Macros (avoid when possible): `MACRO_NAME`

### Code Documentation

- Document all public APIs using Doxygen-style comments
- Include explanations of perspective-dependent behavior where relevant
- Document scale-specific behaviors and transitions

## Technical Challenges

When contributing to Fabric Engine, be mindful of these key technical challenges:

### Performance Optimization

- **Memory Management**: Resources must be efficiently allocated and freed as perspectives shift
- **Spatial Partitioning**: Efficiently identify visible quanta using octrees or similar structures
- **Multi-threading**: Parallelize quantum state updates and perspective calculations
- **Lazy Instantiation**: Generate detailed properties only when needed
- **Garbage Collection**: Clean up quanta beyond perception boundaries

### State Coherence

- **Scale-Appropriate Physics**: Different physical simulations must transition smoothly
- **Propagation Rules**: Changes at one scale must correctly affect other scales
- **Caching Strategies**: Cache rendered states while preserving modifiability
- **Event Bubbling**: Events must propagate properly across quantum hierarchies
- **Serialization**: Quantum states must be saved/loaded with scale relationships intact

### Rendering Challenges

- **Level of Detail Transitions**: Avoid visual popping between detail levels
- **Adaptive Tessellation**: Dynamically adjust geometry complexity based on perspective
- **Floating Point Precision**: Handle vast scale differences without numerical errors
- **Coordinate System Management**: Transform between local and global coordinate spaces
- **Streaming Architecture**: Asynchronously load assets as perspective shifts

## Pull Request Process

1. Ensure your code follows our coding guidelines
2. Add or update tests for your changes
3. Update documentation as needed
4. Submit a pull request with a clear description of your changes
5. Respond to feedback from code reviewers

## Running Tests

Fabric Engine uses a comprehensive test suite. Run tests with:

```bash
cd build
make test
```

Run specific tests with:

```bash
cd build
ctest -R "TestName"
```

## Community

Join our community channels to ask questions and get help:

- [GitHub Discussions](https://github.com/yourusername/fabric/discussions)
- [Discord](https://discord.gg/fabricengine)

## License

By contributing to Fabric Engine, you agree that your contributions will be licensed under the project's [MIT License](../LICENSE).