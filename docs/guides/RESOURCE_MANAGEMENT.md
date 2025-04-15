# Resource Management in Fabric Engine

## Introduction

The Fabric Engine employs a sophisticated, graph-based approach to resource management that resolves common challenges in game engines and real-time applications. This document outlines the architecture and implementation details of this system.

## Architecture Overview

Fabric's resource management consists of three primary components:

1. **Resource Base System**: Defines the common interface and lifecycle
2. **CoordinatedGraph**: Provides the thread-safe dependency tracking structure with intent-based locking
3. **ResourceHub**: Ties everything together with practical workflows

These components create a unified system for resource loading, dependency tracking, and memory management that's both thread-safe and highly efficient.

## Core Resource System

### The Resource Interface

```cpp
class Resource {
public:
    explicit Resource(std::string id);
    virtual ~Resource() = default;
    
    // Core interface
    const std::string& getId() const;
    ResourceState getState() const;
    virtual size_t getMemoryUsage() const = 0;
    
    // Lifecycle management
    bool load();
    void unload();
    
protected:
    // Implemented by derived classes
    virtual bool loadImpl() = 0;
    virtual void unloadImpl() = 0;
    
private:
    std::string id_;
    ResourceState state_;
    mutable std::mutex mutex_;
    int loadCount_ = 0;
};
```

### Resource Types and Factory

The ResourceFactory uses a type registry pattern for extensibility:

```cpp
// Registration
ResourceFactory::registerType<TextureResource>("texture", 
    [](const std::string& id) {
        return std::make_shared<TextureResource>(id);
    }
);

// Creation
auto resource = ResourceFactory::create("texture", "textures/grass.png");
```

This allows new resource types to be added without modifying the core system.

## Resource Handles

Fabric uses a reference-counted handle pattern for safe resource access:

```cpp
template <typename T>
class ResourceHandle {
public:
    ResourceHandle() = default;
    ResourceHandle(std::shared_ptr<T> resource, ResourceManager* manager);
    
    T* get() const;
    T* operator->() const;
    explicit operator bool() const;
    
private:
    std::shared_ptr<T> resource_;
    ResourceManager* manager_ = nullptr;
};
```

This handle system provides:
- Type safety through templates
- Reference counting via shared_ptr
- Null safety with bool conversion
- Convenient arrow operator access

## CoordinatedGraph Integration

The ResourceHub uses a coordinated graph to track dependencies with intent-based locking:

```cpp
class ResourceHub {
private:
    CoordinatedGraph<std::shared_ptr<Resource>> resourceGraph_;
    // ...
};
```

### Resource Nodes and Dependency Edges

Each resource is a node in the graph, with dependencies represented as edges:

- **Node**: A resource (texture, mesh, shader, etc.)
- **Edge**: A dependency relationship (material → texture)

This structure enables:
- Efficient dependency traversal
- Proper load ordering
- Safe unloading verification

## Loading Patterns

### Synchronous Loading

```cpp
auto textureHandle = ResourceHub::instance()
    .load<TextureResource>("texture", "textures/grass.png");

if (textureHandle) {
    // Use the resource
    renderer.setTexture(textureHandle);
}
```

### Asynchronous Loading

```cpp
ResourceHub::instance().loadAsync<TextureResource>(
    "texture", "textures/background.jpg",
    ResourcePriority::Low,
    [](ResourceHandle<TextureResource> handle) {
        if (handle) {
            ui.setBackgroundTexture(handle);
        }
    }
);
```

### Dependency Management

```cpp
// Material depends on various textures
manager.addDependency("material:metal", "texture:metal_diffuse");
manager.addDependency("material:metal", "texture:metal_normal");
manager.addDependency("material:metal", "texture:metal_specular");

// Loading the material will ensure all dependencies load first
auto material = manager.load<Material>("material", "metal");
```

## Memory Management

### Budget-Based Unloading

```cpp
// Set memory budget for resources
manager.setMemoryBudget(512 * 1024 * 1024); // 512 MB

// System automatically unloads least-recently-used resources when needed
size_t evicted = manager.enforceMemoryBudget();
logger.info("Evicted {} resources to stay within budget", evicted);
```

### Eviction Strategy

The ResourceHub uses a sophisticated LRU eviction algorithm:

1. Resources with dependents are never evicted
2. Resources with external references are never evicted
3. Least recently accessed resources are evicted first
4. Unload continues until memory usage is under budget

```cpp
// Implementation excerpt
for (const auto& id : allResourceIds) {
    auto node = resourceGraph_.getNode(id);
    if (node && resource->getState() == ResourceState::Loaded && 
        resource.use_count() == 1 && resourceGraph_.getInEdges(id).empty()) {
        // Safe to evict - no references, no dependents
        candidates.push_back({
            id,
            node->getLastAccessTime(),
            resource->getMemoryUsage()
        });
    }
}
```

## Threading Model

### Worker Thread Pool

The ResourceHub uses a configurable thread pool for background loading:

```cpp
// Configure worker threads
unsigned int threadCount = std::thread::hardware_concurrency();
manager.setWorkerThreadCount(threadCount);
```

### Priority-Based Loading

Resources are loaded according to priority:

```cpp
enum class ResourcePriority {
    Lowest,  // Background preloading
    Low,     // Non-essential resources
    Normal,  // Default priority
    High,    // Important resources
    Highest  // Critical, load immediately
};
```

This ensures critical resources load before less important ones during heavy loading periods.

## Implementation Considerations

### Von Neumann Architecture Optimizations

Fabric's resource system is optimized for modern CPU architectures:

1. **Cache Locality**: Related resources stored together where possible
2. **Batched Operations**: Group similar operations to maximize throughput
3. **Lock Minimization**: Hold locks for minimal time to reduce CPU stalls
4. **Memory Pre-allocation**: Reduce fragmentation and allocation overhead
5. **Lock-Free Paths**: Critical operations use atomic operations where possible

### Performance Characteristics

The system delivers excellent performance characteristics:

- O(1) resource lookup by ID
- O(log n) priority-based loading
- O(out-degree) dependency traversal
- Near-linear scaling with additional CPU cores

### Memory Overhead

The graph structure adds minimal overhead:
- ~40 bytes per node (excluding resource data)
- ~16 bytes per edge
- Lock objects sized for your platform's mutex implementation

## Best Practices

1. **Resource IDs**: Use consistent, hierarchical ID schemes (e.g., "texture:grass")
2. **Explicit Dependencies**: Always declare dependencies between resources
3. **Handle Timeouts**: Set reasonable timeouts for synchronous loading operations
4. **Preloading**: Use low-priority preloading during idle times
5. **Memory Budgets**: Set realistic budgets based on target hardware
6. **Resource Reuse**: Share resources where appropriate to reduce memory usage
7. **Error Handling**: Always check if resource handles are valid before use

## Conclusion

Fabric Engine's resource management system represents a significant advancement over traditional approaches. By combining graph theory with modern concurrency techniques, it achieves both correctness (preventing invalid states) and performance (maximizing parallel operations) while maintaining a clean, easy-to-use API for developers.