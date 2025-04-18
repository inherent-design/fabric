# Unified Fabric Codebase Refactoring Plan

## Overview

This document outlines a comprehensive plan for completely refactoring the Fabric codebase. This is a clean-break approach with no backward compatibility concerns - we're prioritizing the best design for the future rather than incremental transitions.

### Primary Goals

1. Break down monolithic classes into focused, single-responsibility components
2. Create explicit, predictable APIs with clear error handling
3. Implement strong type safety throughout the system
4. Build a trait-based rendering system supporting multiple backends
5. Establish modern concurrency patterns with proper safety guarantees
6. Organize code into logical, maintainable modules
7. Remove all implicit behaviors and side effects

## Phase 1: Reset Core Architecture

### New Directory Structure

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

### Clean Break Approach

1. Create new interfaces and implementations from scratch
2. Implement one component at a time in the new architecture
3. No compatibility layers - design for clarity and correctness
4. Phase out old components completely as new ones are implemented
5. Comprehensive tests for each new component

## Phase 2: Modern Error Handling & Type System

### Error Handling System

```cpp
// Error handling based on explicit Result<T> types
class Error {
public:
    enum class Code {
        // System error codes
        ResourceNotFound,
        ResourceLoadFailed,
        InvalidArgument,
        OperationTimedOut,
        InternalError,
        // More specific codes...
    };
    
    Error(Code code, std::string message);
    
    // Add context information
    Error& withContext(std::string key, std::string value);
    
    // Access error details
    Code getCode() const;
    const std::string& getMessage() const;
    const std::map<std::string, std::string>& getContext() const;
    std::string toString() const;
    
private:
    Code code_;
    std::string message_;
    std::map<std::string, std::string> context_;
};

// Result type for error handling without exceptions
template <typename T>
class Result {
public:
    static Result<T> success(T value);
    static Result<T> failure(Error error);
    
    bool isSuccess() const;
    const T& value() const;  // Panics if error
    const Error& error() const;  // Panics if success
    
    // Functional operators for composition
    template <typename Func>
    auto map(Func&& func) -> Result<std::invoke_result_t<Func, T>>;
    
    template <typename Func>
    auto flatMap(Func&& func) -> std::invoke_result_t<Func, T>;
    
private:
    bool success_;
    std::variant<T, Error> data_;
};

// Example usage
Result<Resource> loadResource(ResourceId id) {
    if (!resourceExists(id)) {
        return Result<Resource>::failure(
            Error(Error::Code::ResourceNotFound, "Resource not found")
                .withContext("resourceId", id.value()));
    }
    
    // Successful case
    return Result<Resource>::success(Resource(id));
}
```

### Strong Type System

```cpp
// Strong typing for identifiers
template <typename Tag>
class Id {
public:
    explicit Id(std::string value) : value_(std::move(value)) {}
    
    const std::string& value() const { return value_; }
    
    bool operator==(const Id& other) const { return value_ == other.value_; }
    bool operator!=(const Id& other) const { return value_ != other.value_; }
    
    // Hash support
    struct Hash {
        size_t operator()(const Id& id) const {
            return std::hash<std::string>{}(id.value_);
        }
    };
    
private:
    std::string value_;
};

// Define specific ID types
struct ResourceTag {};
struct EntityTag {};
struct ComponentTag {};

using ResourceId = Id<ResourceTag>;
using EntityId = Id<EntityTag>;
using ComponentId = Id<ComponentTag>;

// Non-nullable pointer wrapper
template <typename T>
class NonNull {
public:
    explicit NonNull(T* ptr) {
        if (!ptr) throw std::logic_error("Null pointer");
        ptr_ = ptr;
    }
    
    T* get() const { return ptr_; }
    T& operator*() const { return *ptr_; }
    T* operator->() const { return ptr_; }
    
private:
    T* ptr_;
};

// Type-safe flags using enums
template <typename E>
class Flags {
public:
    using UnderlyingType = std::underlying_type_t<E>;
    
    constexpr Flags() : value_(0) {}
    constexpr Flags(E flag) : value_(static_cast<UnderlyingType>(flag)) {}
    
    constexpr bool hasFlag(E flag) const;
    constexpr Flags& setFlag(E flag, bool on = true);
    
    // Operators for combining flags
    constexpr Flags operator|(E flag) const;
    constexpr Flags operator&(E flag) const;
    
private:
    UnderlyingType value_;
};
```

## Phase 3: Advanced Logging System

```cpp
// Structured, context-aware logging
class LogContext {
public:
    LogContext() = default;
    
    template <typename T>
    LogContext& with(std::string key, T value);
    
    std::string toJson() const;
    
private:
    std::map<std::string, std::string> data_;
};

class Logger {
public:
    enum class Level { Debug, Info, Warning, Error, Critical };
    
    // Categorized logging
    class Category {
    public:
        explicit Category(std::string name);
        
        void log(Level level, std::string message, 
                 const LogContext& context = {});
        
        void debug(std::string message, const LogContext& context = {});
        void info(std::string message, const LogContext& context = {});
        void warning(std::string message, const LogContext& context = {});
        void error(std::string message, const LogContext& context = {});
        void critical(std::string message, const LogContext& context = {});
        
    private:
        std::string name_;
        std::atomic<bool> enabled_ = true;
    };
    
    static Category& getCategory(std::string name);
    
    // JSON structured logging format
    static void enableJsonFormat(bool enable);
    
    // Log filtering
    static void setGlobalLevel(Level level);
    static void setCategoryLevel(std::string category, Level level);
    
private:
    static std::map<std::string, std::unique_ptr<Category>> categories_;
    static Level globalLevel_;
    static bool jsonFormat_;
};

// Pre-defined categories
namespace LogCategories {
    extern Logger::Category Core;
    extern Logger::Category Resources;
    extern Logger::Category Rendering;
    extern Logger::Category Physics;
}
```

## Phase 4: Thread Safety and Concurrency

```cpp
// Thread safety primitive
template <typename T>
class ThreadSafe {
public:
    ThreadSafe() = default;
    explicit ThreadSafe(T value) : value_(std::move(value)) {}
    
    // RAII lock guards
    class ReadGuard {
    public:
        const T& get() const;
        const T* operator->() const;
        
    private:
        std::shared_lock<std::shared_mutex> lock_;
        const T& value_;
    };
    
    class WriteGuard {
    public:
        T& get();
        T* operator->(); 
        
    private:
        std::unique_lock<std::shared_mutex> lock_;
        T& value_;
    };
    
    // Modern timeout-based locking
    Result<ReadGuard> tryReadLock(std::chrono::milliseconds timeout = std::chrono::milliseconds(100));
    Result<WriteGuard> tryWriteLock(std::chrono::milliseconds timeout = std::chrono::milliseconds(100));
    
    // Functional access patterns
    template <typename Func>
    auto read(Func&& func, std::chrono::milliseconds timeout = std::chrono::milliseconds(100))
        -> Result<std::invoke_result_t<Func, const T&>>;
    
    template <typename Func>
    auto update(Func&& func, std::chrono::milliseconds timeout = std::chrono::milliseconds(100))
        -> Result<std::invoke_result_t<Func, T&>>;
    
private:
    T value_{};
    mutable std::shared_mutex mutex_;
};

// Task-based concurrency wrapper
class Task {
public:
    template <typename T>
    class Promise {
    public:
        // Chain operations
        template <typename Func>
        auto then(Func&& func) -> Promise<std::invoke_result_t<Func, T>>;
        
        // Handle errors
        template <typename Func>
        auto onError(Func&& func) -> Promise<T>;
        
        // Wait for result
        Result<T> get();
        Result<T> getWithTimeout(std::chrono::milliseconds timeout);
        
    private:
        std::future<Result<T>> future_;
    };
    
    // Create async tasks
    template <typename Func, typename... Args>
    static auto run(Func&& func, Args&&... args)
        -> Promise<std::invoke_result_t<Func, Args...>>;
    
    template <typename Func, typename... Args>
    static auto runDelayed(std::chrono::milliseconds delay, Func&& func, Args&&... args)
        -> Promise<std::invoke_result_t<Func, Args...>>;
};
```

## Phase 5: Trait-Based Rendering System

```cpp
// Base rendering interfaces
class IRenderableObject {
public:
    virtual ~IRenderableObject() = default;
    
    // Core rendering interface
    virtual void render(class IRenderContext& context) const = 0;
    
    // Required properties
    virtual bool isVisible() const = 0;
    virtual Bounds getBounds() const = 0;
    virtual Transform getWorldTransform() const = 0;
    
    // Optional properties with default implementations
    virtual Optional<Color> getColor() const { return std::nullopt; }
    virtual Optional<ResourceId> getTexture() const { return std::nullopt; }
};

// Rendering context interface
class IRenderContext {
public:
    virtual ~IRenderContext() = default;
    
    // Drawing operations
    virtual void drawRect(const Rect& rect, const Color& color) = 0;
    virtual void drawTexture(ResourceId textureId, const Rect& destRect) = 0;
    virtual void drawText(const std::string& text, const Point& position, const TextStyle& style) = 0;
    
    // Transform management
    virtual void setTransform(const Transform& transform) = 0;
    virtual void pushTransform() = 0;
    virtual void popTransform() = 0;
    
    // State management
    virtual void setClipRect(const Optional<Rect>& clipRect) = 0;
    virtual void setBlendMode(BlendMode mode) = 0;
};

// Renderer interface
class IRenderer {
public:
    virtual ~IRenderer() = default;
    
    // Core operations
    virtual Result<void> initialize(const RendererConfig& config) = 0;
    virtual void shutdown() = 0;
    virtual void beginFrame() = 0;
    virtual void endFrame() = 0;
    virtual void present() = 0;
    
    // Create context for rendering 
    virtual std::unique_ptr<IRenderContext> createContext() = 0;
    
    // Resource management
    virtual Result<ResourceId> createTexture(int width, int height, TextureFormat format) = 0;
    virtual Result<ResourceId> loadTexture(const std::string& path) = 0;
    virtual void destroyTexture(ResourceId textureId) = 0;
};

// Concrete implementations
class SDL3Renderer : public IRenderer {
public:
    // IRenderer implementation
    Result<void> initialize(const RendererConfig& config) override;
    void shutdown() override;
    void beginFrame() override;
    void endFrame() override;
    void present() override;
    std::unique_ptr<IRenderContext> createContext() override;
    Result<ResourceId> createTexture(int width, int height, TextureFormat format) override;
    Result<ResourceId> loadTexture(const std::string& path) override;
    void destroyTexture(ResourceId textureId) override;
    
private:
    SDL_Renderer* renderer_ = nullptr;
    std::unordered_map<ResourceId, SDL_Texture*, ResourceId::Hash> textures_;
};

class HTMLCanvasRenderer : public IRenderer {
public:
    // IRenderer implementation
    Result<void> initialize(const RendererConfig& config) override;
    void shutdown() override;
    void beginFrame() override;
    void endFrame() override;
    void present() override;
    std::unique_ptr<IRenderContext> createContext() override;
    Result<ResourceId> createTexture(int width, int height, TextureFormat format) override;
    Result<ResourceId> loadTexture(const std::string& path) override;
    void destroyTexture(ResourceId textureId) override;
    
private:
    // Canvas-specific implementation details
};
```

## Phase 6: Entity-Component System

```cpp
// Entity-Component System
class Entity {
public:
    explicit Entity(EntityId id);
    
    EntityId getId() const;
    
    // Component management
    template <typename T, typename... Args>
    Result<T*> addComponent(Args&&... args);
    
    template <typename T>
    Result<T*> getComponent();
    
    bool removeComponent(ComponentId id);
    
    // Lifecycle methods
    void update(float deltaTime);
    
private:
    EntityId id_;
    std::unordered_map<ComponentId, std::unique_ptr<Component>, ComponentId::Hash> components_;
};

// Component base class
class Component {
public:
    explicit Component(ComponentId id);
    virtual ~Component() = default;
    
    ComponentId getId() const;
    
    // Lifecycle methods
    virtual void initialize() = 0;
    virtual void update(float deltaTime) = 0;
    virtual void shutdown() = 0;
    
    // Entity ownership
    void setOwner(Entity* entity);
    Entity* getOwner() const;
    
private:
    ComponentId id_;
    Entity* owner_ = nullptr;
};

// Scene management
class Scene {
public:
    explicit Scene(std::string name);
    
    // Entity management
    Result<EntityId> createEntity(std::string name = "");
    bool destroyEntity(EntityId id);
    Result<Entity*> getEntity(EntityId id);
    
    // Scene lifecycle
    void update(float deltaTime);
    void render(IRenderer& renderer);
    
    // Rendering
    void collectRenderables(std::vector<IRenderableObject*>& renderables);
    
private:
    std::string name_;
    std::unordered_map<EntityId, std::unique_ptr<Entity>, EntityId::Hash> entities_;
};
```

## Phase 7: Resource Management System

```cpp
// Resource management
class Resource {
public:
    explicit Resource(ResourceId id);
    virtual ~Resource() = default;
    
    ResourceId getId() const;
    ResourceState getState() const;
    
    // Core interface
    virtual size_t getMemoryUsage() const = 0;
    virtual Result<void> load() = 0;
    virtual void unload() = 0;
    
protected:
    ResourceId id_;
    ResourceState state_ = ResourceState::Unloaded;
};

// Resource loader - focused responsibility
class ResourceLoader {
public:
    template <typename T>
    Result<ResourceHandle<T>> load(ResourceId id);
    
    template <typename T>
    Task::Promise<ResourceHandle<T>> loadAsync(ResourceId id, ResourcePriority priority);
    
    void preload(const std::vector<ResourceId>& ids, ResourcePriority priority);
    
private:
    // Implementation details
};

// Resource dependency manager
class ResourceDependencyManager {
public:
    Result<void> addDependency(ResourceId dependent, ResourceId dependency);
    Result<void> removeDependency(ResourceId dependent, ResourceId dependency);
    std::vector<ResourceId> getDependencies(ResourceId id);
    std::vector<ResourceId> getDependents(ResourceId id);
    
private:
    // Implementation using CoordinatedGraph
    CoordinatedGraph<std::shared_ptr<Resource>> resourceGraph_;
};

// Memory budget enforcement
class ResourceMemoryManager {
public:
    void setMemoryBudget(size_t bytes);
    size_t getMemoryUsage() const;
    Result<size_t> enforceMemoryBudget();
    
    // Eviction priority methods
    void setResourcePriority(ResourceId id, ResourcePriority priority);
    
private:
    size_t memoryBudget_ = 512 * 1024 * 1024; // Default 512MB
    // LRU tracking, priority management
};

// Thread pool for background loading
class ResourceThreadPool {
public:
    void setThreadCount(unsigned int count);
    void queueRequest(ResourceLoadRequest request);
    bool isShutdown() const;
    void shutdown();
    
private:
    std::vector<std::thread> threads_;
    ThreadSafeQueue<ResourceLoadRequest> queue_;
    std::atomic<bool> shutdown_ = false;
};
```

## Phase 8: Coordinated Graph Refactoring

```cpp
// Split into focused components
template <typename T>
class GraphNode {
public:
    const T& getData() const;
    void setData(const T& data);
    std::chrono::steady_clock::time_point getLastAccessTime() const;
    void updateAccessTime();
    
private:
    T data_;
    std::chrono::steady_clock::time_point lastAccessTime_;
};

// Lock management component
template <typename T>
class GraphLock {
public:
    enum class Intent { Read, NodeModify, GraphStructure };
    
    class Guard {
    public:
        bool isLocked() const;
        void release();
        
    private:
        std::variant<std::shared_lock<std::shared_mutex>, 
                   std::unique_lock<std::shared_mutex>> lock_;
        bool locked_ = false;
    };
    
    Result<Guard> tryLock(Intent intent, std::chrono::milliseconds timeout);
    
private:
    std::shared_mutex mutex_;
};

// Graph traversal algorithms
template <typename T, typename KeyType>
class GraphTraversal {
public:
    using VisitFunc = std::function<void(const KeyType&, const T&)>;
    
    void bfs(const KeyType& startKey, VisitFunc visit);
    void dfs(const KeyType& startKey, VisitFunc visit);
    Result<std::vector<KeyType>> topologicalSort();
    bool detectCycles();
    
private:
    std::unordered_map<KeyType, std::vector<KeyType>> adjacencyList_;
};

// Main graph container
template <typename T, typename KeyType = std::string>
class CoordinatedGraph {
public:
    // Node operations
    Result<void> addNode(const KeyType& key, const T& data);
    Result<void> removeNode(const KeyType& key);
    Result<T> getNodeData(const KeyType& key);
    
    // Edge operations
    Result<void> addEdge(const KeyType& from, const KeyType& to);
    Result<void> removeEdge(const KeyType& from, const KeyType& to);
    Result<std::vector<KeyType>> getOutEdges(const KeyType& key);
    Result<std::vector<KeyType>> getInEdges(const KeyType& key);
    
    // Traversal operations
    Result<void> forEachNode(std::function<void(const KeyType&, const T&)> func);
    Result<std::vector<KeyType>> topologicalSort();
    
private:
    GraphLock<T> lock_;
    std::unordered_map<KeyType, GraphNode<T>> nodes_;
    std::unordered_map<KeyType, std::vector<KeyType>> outEdges_;
    std::unordered_map<KeyType, std::vector<KeyType>> inEdges_;
    GraphTraversal<T, KeyType> traversal_;
};
```

## Implementation Timeline

| Phase | Tasks | Time Frame |
|-------|-------|------------|
| 1 | Reset core architecture, create new directory structure | Week 1 |
| 2 | Implement error handling & type system | Week 2 |
| 3 | Build logging system | Week 3 |
| 4 | Develop thread safety primitives | Week 4 |
| 5 | Create rendering trait system | Weeks 5-6 |
| 6 | Build entity-component system | Weeks 7-8 |
| 7 | Implement resource management | Weeks 9-10 |
| 8 | Refactor coordinated graph | Weeks 11-12 |
| 9 | Testing and integration | Weeks 13-14 |

## Implementation Strategy

1. **Complete rewrite approach:**
   - Don't maintain backward compatibility
   - Build clean new implementation in parallel
   - Focus on getting the design right

2. **Test-first development:**
   - Create unit tests for each component before implementation
   - Ensure 100% test coverage for new code
   - Automated test runners for continuous validation

3. **Radical simplification:**
   - Remove all singleton patterns in favor of dependency injection
   - No global state except for logging system
   - Explicit function parameters over implicit dependencies

4. **Clear ownership model:**
   - Use modern C++ ownership semantics consistently
   - Prefer value types and RAII over manual memory management
   - Clear documentation of ownership responsibility

5. **Compile-time safety:**
   - Use static analysis tools to catch issues early
   - Design APIs that make incorrect usage impossible
   - Extensive use of static_assert for compile-time validation

## Tangible Benefits

This refactoring delivers several concrete benefits:

1. **Improved error handling:**
   - No silent failures or hidden side effects
   - Rich error context for debugging
   - Composable error handling with Result type

2. **Stronger type safety:**
   - Prevents mixing of different ID types
   - No accidental null pointers
   - Type-safe flags and enums

3. **Better concurrency:**
   - Thread-safety guarantees via clean abstractions
   - Impossible to forget locking
   - Deadlock prevention via timeouts

4. **Backend flexibility:**
   - Clean separation of rendering logic
   - Multiple backends without code changes
   - Testable with mock renderers

5. **Performance improvements:**
   - Focused components with better cache locality
   - Optimized memory usage via RAII
   - Reduced overhead from unnecessary locking

6. **Developer experience:**
   - Explicit APIs that are easier to understand
   - Better error messages and logging
   - Self-documenting code structure

## Conclusion

This aggressive refactoring plan prioritizes creating the best possible architecture for the future without the constraints of backward compatibility. By completely rebuilding core systems with modern C++ idioms and explicit designs, we'll create a more maintainable, safer, and more powerful codebase.

The trait-based approach to rendering and clean separation of responsibilities will enable multiple backends while keeping the core logic platform-agnostic. The strong type system and explicit error handling will catch issues early and make debugging simpler.

Although this approach requires more upfront investment than an incremental refactoring, it will yield a vastly superior architecture that will be easier to maintain and extend for years to come.