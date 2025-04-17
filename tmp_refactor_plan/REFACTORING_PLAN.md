# Fabric Codebase Refactoring Plan

## Overview

This document outlines a comprehensive plan for refactoring the Fabric codebase to improve its structure, maintainability, and extensibility. The primary goals are:

1. Break down large functionalities into smaller, more focused files
2. Reorganize components into logical modules
3. Abstract hard-coded implementations into reusable units
4. Create functional patterns and mixins for common behavior
5. Update build system to reflect the new structure

## Phase 1: Code Analysis and Preparation

### Component Analysis

We've identified the following key components that would benefit from refactoring:

1. **ResourceHub**: A large class with multiple responsibilities:
   - Resource loading
   - Dependency management
   - Memory management
   - Thread management

2. **CoordinatedGraph**: A complex utility with multiple concerns:
   - Graph structure management
   - Traversal algorithms
   - Thread-safe operations
   - Lock management

3. **Event System**: Currently scattered across multiple files:
   - Event creation
   - Event dispatching
   - Event handling

### Directory Structure Changes

We'll create a more granular directory structure:

```
include/fabric/
  ├── core/
  │   ├── parser/       # Moved from top-level include/
  │   ├── ui/           # Moved from top-level include/
  │   └── resource/     # New submodule for resource management
  ├── utils/
      ├── concurrency/  # Thread-safe patterns and utilities
      ├── memory/       # Memory management utilities
      ├── graph/        # Graph algorithms and structures
      └── patterns/     # Reusable design patterns
```

## Phase 2: ResourceHub Refactoring

The current ResourceHub class will be split into smaller, more focused components:

### ResourceLoader
Handles loading resources synchronously and asynchronously.

```cpp
class ResourceLoader {
public:
    template <typename T>
    ResourceHandle<T> load(const std::string &typeId, const std::string &resourceId);
    
    template <typename T>
    void loadAsync(const std::string &typeId, const std::string &resourceId,
                  ResourcePriority priority,
                  std::function<void(ResourceHandle<T>)> callback);
    
    void preload(const std::vector<std::string> &typeIds,
                const std::vector<std::string> &resourceIds,
                ResourcePriority priority);
private:
    // Implementation details
};
```

### ResourceDependencyManager
Manages relationships between resources using a graph structure.

```cpp
class ResourceDependencyManager {
public:
    bool addDependency(const std::string &dependentId, const std::string &dependencyId);
    bool removeDependency(const std::string &dependentId, const std::string &dependencyId);
    bool removeResource(const std::string &resourceId, bool cascade = false);
    // Other dependency management methods
private:
    CoordinatedGraph<std::shared_ptr<Resource>> resourceGraph_;
};
```

### ResourceMemoryManager
Handles memory budgeting and resource eviction.

```cpp
class ResourceMemoryManager {
public:
    void setMemoryBudget(size_t bytes);
    size_t getMemoryBudget() const;
    size_t getMemoryUsage() const;
    size_t enforceMemoryBudget();
private:
    // Implementation details
};
```

### ResourceThreadPool
Manages worker threads for asynchronous resource loading.

```cpp
class ResourceThreadPool {
public:
    void queueRequest(const ResourceLoadRequest &request);
    unsigned int getWorkerThreadCount() const;
    void setWorkerThreadCount(unsigned int count);
    // Other thread management methods
private:
    // Implementation details
};
```

### Refactored ResourceHub
The ResourceHub class will become a facade that coordinates between these components:

```cpp
class ResourceHub {
public:
    static ResourceHub &instance();
    
    // Delegating methods
    template <typename T>
    ResourceHandle<T> load(const std::string &typeId, const std::string &resourceId) {
        return resourceLoader_->load<T>(typeId, resourceId);
    }
    
    // Other delegating methods
    
private:
    std::shared_ptr<ResourceLoader> resourceLoader_;
    std::shared_ptr<ResourceDependencyManager> dependencyManager_;
    std::shared_ptr<ResourceMemoryManager> memoryManager_;
    std::shared_ptr<ResourceThreadPool> threadPool_;
};
```

## Phase 3: Utils Refactoring

### Create Thread-Safe Utilities

Extract thread-safe patterns into dedicated utilities:

```cpp
// Thread-safe queue implementation
template <typename T>
class ThreadSafeQueue {
public:
    void push(const T& item);
    bool tryPop(T& item);
    template <typename Predicate>
    bool waitAndPop(T& item, Predicate predicate);
    // Other methods
};

// Thread pool implementation
class ThreadPool {
public:
    ThreadPool(unsigned int threadCount = std::thread::hardware_concurrency());
    void enqueue(std::function<void()> task);
    // Other methods
};
```

### Create Locking Utilities

Extract locking patterns into dedicated utilities:

```cpp
// Timeout-protected locking
template <typename Mutex>
class TimeoutLock {
public:
    TimeoutLock(Mutex& mutex, std::chrono::milliseconds timeout);
    bool isLocked() const;
    void release();
    // Other methods
};
```

### Refactor CoordinatedGraph

Split CoordinatedGraph into smaller components:

```cpp
// Graph node
template <typename T>
class GraphNode {
public:
    const T& getData() const;
    T& getData();
    // Other methods
};

// Graph traversal utilities
template <typename T, typename KeyType>
class GraphTraversal {
public:
    void bfs(const KeyType& startKey, std::function<void(const KeyType&, const T&)> visitFunc);
    void dfs(const KeyType& startKey, std::function<void(const KeyType&, const T&)> visitFunc);
    std::vector<KeyType> topologicalSort();
    // Other methods
};
```

## Phase 4: Moving Parser and UI Into Core

### Parser Integration

1. Move files to core/parser
2. Update includes in all files that reference parser components
3. Update forward declarations and namespaces

### UI Integration

1. Move files to core/ui
2. Update includes in all files that reference UI components
3. Update forward declarations and namespaces

## Phase 5: CMake Updates

The build system will need significant updates to reflect the new structure:

```cmake
# Core library components
set(FABRIC_CORE_COMMON_SOURCE_FILES
    src/core/Component.cc
    src/core/Event.cc
    # Other core files
)

# Resource management subsystem
set(FABRIC_CORE_RESOURCE_SOURCE_FILES
    src/core/resource/Resource.cc
    src/core/resource/ResourceHub.cc
    src/core/resource/ResourceLoader.cc
    src/core/resource/ResourceDependencyManager.cc
    src/core/resource/ResourceMemoryManager.cc
    src/core/resource/ResourceThreadPool.cc
)

# Parser subsystem
set(FABRIC_CORE_PARSER_SOURCE_FILES
    src/core/parser/ArgumentParser.cc
    src/core/parser/SyntaxTree.cc
    src/core/parser/Token.cc
)

# UI subsystem
set(FABRIC_CORE_UI_SOURCE_FILES
    src/core/ui/WebView.cc
)

# Utils common files
set(FABRIC_UTILS_COMMON_SOURCE_FILES
    src/utils/ErrorHandling.cc
    src/utils/Logging.cc
    src/utils/Utils.cc
)

# Utils concurrency files
set(FABRIC_UTILS_CONCURRENCY_SOURCE_FILES
    src/utils/concurrency/ThreadPool.cc
    src/utils/concurrency/ThreadSafeQueue.cc
)

# Utils graph files
set(FABRIC_UTILS_GRAPH_SOURCE_FILES
    src/utils/graph/CoordinatedGraph.cc
    src/utils/graph/GraphNode.cc
    src/utils/graph/GraphTraversal.cc
)

# Main fabric libraries
add_library(fabric_utils STATIC 
    ${FABRIC_UTILS_COMMON_SOURCE_FILES}
    ${FABRIC_UTILS_CONCURRENCY_SOURCE_FILES}
    ${FABRIC_UTILS_GRAPH_SOURCE_FILES}
    # Other utils files
)

add_library(fabric_core STATIC 
    ${FABRIC_CORE_COMMON_SOURCE_FILES}
    ${FABRIC_CORE_RESOURCE_SOURCE_FILES}
    ${FABRIC_CORE_PARSER_SOURCE_FILES}
    ${FABRIC_CORE_UI_SOURCE_FILES}
)

target_link_libraries(fabric_core PUBLIC fabric_utils)
```

## Phase 6: Testing Updates

Tests must be updated to match the new structure:

```cmake
# Unit tests for resource system
set(FABRIC_RESOURCE_TESTS
    tests/unit/core/resource/ResourceTest.cc
    tests/unit/core/resource/ResourceHubTest.cc
    tests/unit/core/resource/ResourceLoaderTest.cc
    tests/unit/core/resource/ResourceDependencyManagerTest.cc
    tests/unit/core/resource/ResourceMemoryManagerTest.cc
    tests/unit/core/resource/ResourceThreadPoolTest.cc
)

# Unit tests for utils concurrency
set(FABRIC_UTILS_CONCURRENCY_TESTS
    tests/unit/utils/concurrency/ThreadPoolTest.cc
    tests/unit/utils/concurrency/ThreadSafeQueueTest.cc
)

# Add all unit tests to the UnitTests target
target_sources(UnitTests PRIVATE
    ${FABRIC_CORE_TESTS}
    ${FABRIC_RESOURCE_TESTS}
    ${FABRIC_PARSER_TESTS}
    ${FABRIC_UI_TESTS}
    ${FABRIC_UTILS_TESTS}
    ${FABRIC_UTILS_CONCURRENCY_TESTS}
    ${FABRIC_UTILS_GRAPH_TESTS}
    # Other test files
)
```

## Implementation Schedule

1. **Week 1**: Set up new directory structure and create skeleton classes
2. **Week 2**: Refactor ResourceHub into smaller components
3. **Week 3**: Update utils components and create new utilities
4. **Week 4**: Move parser and UI into core
5. **Week 5**: Update build system and fix any issues
6. **Week 6**: Update tests and documentation

## Migration Strategy

To ensure a smooth transition:

1. Use the facade pattern to maintain backward compatibility
2. Keep old interfaces working while new implementations are being developed
3. Implement and test one component at a time
4. Use feature flags to gradually roll out changes
5. Maintain thorough test coverage throughout the process

## Conclusion

This refactoring plan will significantly improve the architecture of the Fabric codebase by:

1. Breaking down large, complex components into smaller, more focused ones
2. Organizing code into logical modules with clear responsibilities
3. Extracting reusable patterns and utilities
4. Improving maintainability and extensibility

The changes will make the codebase easier to understand, debug, and extend without changing its core functionality.