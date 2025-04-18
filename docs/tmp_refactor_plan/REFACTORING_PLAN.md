# Fabric Engine Refactoring Plan

## Overview

This document outlines our approach to refactoring the Fabric codebase. We're adopting a clean-break strategy that prioritizes the best design for the future rather than incremental transitions.

## Goals

1. Break down monolithic classes into focused components
2. Create explicit, predictable APIs with clear error handling
3. Implement strong type safety throughout the system
4. Build a trait-based rendering system supporting multiple backends
5. Establish modern concurrency patterns with proper safety guarantees
6. Organize code into logical, maintainable modules
7. Remove all implicit behaviors and side effects

## Implementation Phases

### Phase 1: Core Architecture

**Focus**: Establish foundation directory structure and base interfaces
**Deliverables**: Directory structure, core interfaces, skeleton implementations
**Timeline**: Week 1

```
include/fabric/
  ├── core/
  │   ├── entity/        # Entity component system
  │   ├── event/         # Event system
  │   ├── resource/      # Resource management
  │   └── scene/         # Scene management
  ├── render/
  │   ├── interface/     # Rendering trait interfaces
  │   ├── sdl/           # SDL3 implementation
  │   └── html/          # HTML/Canvas implementation
  └── utils/
      ├── concurrency/   # Thread-safe utilities
      ├── error/         # Error handling
      ├── graph/         # Graph algorithms
      ├── log/           # Logging system
      └── memory/        # Memory management
```

### Phase 2: Error Handling & Type System

**Focus**: Implement modern error handling and strong typing
**Deliverables**: Result<T>, Error class, strong typing primitives
**Timeline**: Week 2

Key components:
- Explicit Result<T> pattern instead of exceptions
- Rich context for debugging in Error class
- Strong typing for IDs and other primitives
- Non-nullable pointer wrappers

### Phase 3: Logging System

**Focus**: Implement structured, context-aware logging
**Deliverables**: Logger, LogContext, categorized logging
**Timeline**: Week 3

Features:
- JSON structured logging format
- Categorized logging (Core, Resources, Rendering, Physics)
- Context-rich log entries with key-value data
- Configurable verbosity levels

### Phase 4: Thread Safety

**Focus**: Create robust concurrency primitives
**Deliverables**: ThreadSafe<T>, Task-based concurrency, ThreadSafeQueue
**Timeline**: Week 4

Key components:
- Timeout-based locking to prevent deadlocks
- RAII lock guards for automatic cleanup
- Functional access patterns for thread-safe operations
- Promise-based asynchronous task system

### Phase 5: Rendering System

**Focus**: Implement trait-based rendering with multiple backends
**Deliverables**: IRenderableObject, IRenderContext, IRenderer interfaces
**Timeline**: Weeks 5-6

Features:
- Backend-agnostic rendering interfaces
- SDL3 and HTML Canvas implementations
- Resource management for textures
- State management (transforms, clipping, blend modes)

### Phase 6: Entity-Component System

**Focus**: Create modular game object framework
**Deliverables**: Entity, Component, Scene classes
**Timeline**: Weeks 7-8

Features:
- Composition over inheritance for game objects
- Type-safe component access
- Consistent lifecycle management
- Efficient scene organization

### Phase 7: Resource Management

**Focus**: Implement comprehensive resource handling
**Deliverables**: ResourceHub and component managers
**Timeline**: Weeks 9-10

Components:
- ResourceLoader for loading from different sources
- ResourceDependencyManager for tracking dependencies
- ResourceMemoryManager for budget enforcement
- ResourceThreadPool for async loading

### Phase 8: Coordinated Graph

**Focus**: Create thread-safe graph structure with algorithms
**Deliverables**: CoordinatedGraph template and related classes
**Timeline**: Weeks 11-12

Features:
- Intent-based locking (Read, NodeModify, GraphStructure)
- Timeout protection against deadlocks
- Common graph algorithms (BFS, DFS, topological sort)
- Access tracking for LRU policies

## Implementation Strategy

1. **Complete rewrite approach**:
   - Build clean new implementation in parallel
   - Focus on getting the design right
   - No backwards compatibility constraints

2. **Test-first development**:
   - Create unit tests for each component before implementation
   - Ensure thorough test coverage
   - Automated test runners for continuous validation

3. **Radical simplification**:
   - Remove all singleton patterns in favor of dependency injection
   - No global state except for logging system
   - Explicit function parameters over implicit dependencies

4. **Clear ownership model**:
   - Use modern C++ ownership semantics consistently
   - Prefer value types and RAII over manual memory management
   - Clear documentation of ownership responsibility

5. **Compile-time safety**:
   - Use static analysis tools to catch issues early
   - Design APIs that make incorrect usage impossible
   - Extensive use of static_assert for compile-time validation

## Timeline

| Phase | Timeline | Key Deliverables |
|-------|----------|------------------|
| 1. Core Architecture | Week 1 | Directory structure, base interfaces |
| 2. Error Handling | Week 2 | Result<T>, Error, strong types |
| 3. Logging System | Week 3 | Logger, LogContext, categories |
| 4. Thread Safety | Week 4 | ThreadSafe<T>, Task, ThreadSafeQueue |
| 5. Rendering | Weeks 5-6 | Rendering interfaces, backend implementations |
| 6. Entity-Component | Weeks 7-8 | Entity, Component, Scene |
| 7. Resource Management | Weeks 9-10 | ResourceHub and related managers |
| 8. Coordinated Graph | Weeks 11-12 | CoordinatedGraph and algorithms |
| Integration & Testing | Weeks 13-14 | Final integration and testing |

## Expected Benefits

1. **Improved error handling**:
   - No silent failures or hidden side effects
   - Rich error context for debugging
   - Composable error handling with Result type

2. **Stronger type safety**:
   - Prevents mixing of different ID types
   - No accidental null pointers
   - Type-safe flags and enums

3. **Better concurrency**:
   - Thread-safety guarantees via clean abstractions
   - Impossible to forget locking
   - Deadlock prevention via timeouts

4. **Backend flexibility**:
   - Clean separation of rendering logic
   - Multiple backends without code changes
   - Testable with mock renderers

5. **Performance improvements**:
   - Focused components with better cache locality
   - Optimized memory usage via RAII
   - Reduced overhead from unnecessary locking

6. **Developer experience**:
   - Explicit APIs that are easier to understand
   - Better error messages and logging
   - Self-documenting code structure