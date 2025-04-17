# Implementation Notes

## Design Issues and Proposed Solutions

After thorough analysis of the codebase, we've identified several design issues that need to be addressed to improve reliability, maintainability, and testability:

1. **Duplicate Implementations**: Similar functionality implemented across ResourceHub and ResourceManager
2. **Complex Threading**: Thread management logic duplicated and inconsistent
3. **Overly Coupled Components**: ResourceHub directly depends on CoordinatedGraph implementation details
4. **Testing Difficulties**: Tests require direct access to implementation details
5. **Error Handling Inconsistencies**: Different approaches to error handling and recovery

## Proposed Pattern Library

We'll create a pattern library with well-defined, composable abstractions to separate concerns:

### 1. ThreadPoolExecutor

A reusable thread pool implementation for background tasks:

```cpp
class ThreadPoolExecutor {
public:
    // Configuration
    ThreadPoolExecutor(size_t threadCount = std::thread::hardware_concurrency());
    ~ThreadPoolExecutor();
    
    // Core API
    void setThreadCount(size_t count);
    size_t getThreadCount() const;
    
    // Task submission 
    template<typename Func, typename... Args>
    auto submit(Func&& func, Args&&... args) -> std::future<std::invoke_result_t<Func, Args...>>;
    
    // Lifecycle control
    void shutdown(std::chrono::milliseconds timeout = std::chrono::milliseconds(1000));
    bool isShutdown() const;
    
    // Testing support
    void pauseForTesting();
    void resumeAfterTesting();
};
```

### 2. TimeoutLock

A utility for timeout-protected lock acquisition:

```cpp
template<typename MutexType>
class TimeoutLock {
public:
    // For read locks
    static std::optional<std::shared_lock<MutexType>> tryLockShared(
        MutexType& mutex, 
        std::chrono::milliseconds timeout = std::chrono::milliseconds(100)
    );
    
    // For write locks
    static std::optional<std::unique_lock<MutexType>> tryLockUnique(
        MutexType& mutex, 
        std::chrono::milliseconds timeout = std::chrono::milliseconds(100)
    );
    
    // For upgradeable locks (if supported by mutex type)
    static std::optional<std::unique_lock<MutexType>> tryUpgradeLock(
        MutexType& mutex,
        std::shared_lock<MutexType>& sharedLock, 
        std::chrono::milliseconds timeout = std::chrono::milliseconds(100)
    );
};
```

### 3. LifecycleState

A trait-like interface for managed resources:

```cpp
template<typename State, typename Derived>
class LifecycleState {
public:
    // State transitions
    bool transitionTo(State newState);
    State getState() const;
    
    // Hooks for derived classes
    virtual bool onEnterState(State state) = 0;
    virtual void onExitState(State state) = 0;
    
private:
    State state_;
    mutable std::shared_mutex stateMutex_;
};
```

### 4. SafeGraph

A simplified graph implementation with clean separation of concerns:

```cpp
template<typename NodeData, typename NodeKey = std::string>
class SafeGraph {
public:
    // Node operations
    bool addNode(const NodeKey& key, NodeData data);
    bool removeNode(const NodeKey& key);
    bool hasNode(const NodeKey& key) const;
    
    // Edge operations
    bool addEdge(const NodeKey& from, const NodeKey& to);
    bool removeEdge(const NodeKey& from, const NodeKey& to);
    bool hasEdge(const NodeKey& from, const NodeKey& to) const;
    
    // Graph queries
    std::vector<NodeKey> getOutEdges(const NodeKey& key) const;
    std::vector<NodeKey> getInEdges(const NodeKey& key) const;
    std::vector<NodeKey> topologicalSort() const;
    bool hasCycle() const;
    
    // Traversal
    void bfs(const NodeKey& startKey, std::function<void(const NodeKey&, const NodeData&)> visitFunc) const;
    void dfs(const NodeKey& startKey, std::function<void(const NodeKey&, const NodeData&)> visitFunc) const;
    
    // Safe data access
    template<typename Func>
    auto withNodeData(const NodeKey& key, Func&& func) 
        -> std::optional<std::invoke_result_t<Func, NodeData&>>;
        
    template<typename Func>
    auto withNodeDataConst(const NodeKey& key, Func&& func) 
        -> std::optional<std::invoke_result_t<Func, const NodeData&>>;
};
```

### 5. ResourceLifecycle

A component for resource lifecycle management:

```cpp
enum class ResourceState {
    Unloaded,
    Loading,
    Loaded,
    LoadingFailed,
    Unloading
};

template<typename Resource>
class ResourceLifecycle : public LifecycleState<ResourceState, Resource> {
public:
    // Lifecycle operations
    bool load();
    void unload();
    int getLoadCount() const;
    
protected:
    // Implementation hooks
    virtual bool loadImpl() = 0;
    virtual void unloadImpl() = 0;
    
private:
    std::atomic<int> loadCount_{0};
};
```

### 6. MemoryBudget

A component for memory budget management:

```cpp
template<typename Resource>
class MemoryBudget {
public:
    // Configuration
    void setMemoryBudget(size_t bytes);
    size_t getMemoryBudget() const;
    size_t getMemoryUsage() const;
    
    // Enforcement
    size_t enforceMemoryBudget();
    
    // Resource registration
    void registerResource(std::shared_ptr<Resource> resource);
    void unregisterResource(const std::string& resourceId);
    
    // Eviction policy
    using EvictionCandidate = std::pair<std::shared_ptr<Resource>, std::chrono::steady_clock::time_point>;
    void setEvictionPolicy(std::function<std::vector<EvictionCandidate>(
        const std::vector<EvictionCandidate>&, size_t memoryToFree)> policy);
        
private:
    std::atomic<size_t> memoryBudget_{0};
};
```

### 7. AsyncRunner

A utility for timeout-protected async operations:

```cpp
class AsyncRunner {
public:
    // Run a function asynchronously with timeout
    template<typename Func, typename... Args>
    static auto runWithTimeout(
        std::chrono::milliseconds timeout,
        Func&& func, 
        Args&&... args
    ) -> std::optional<std::invoke_result_t<Func, Args...>>;
    
    // Run and log errors
    template<typename Func, typename... Args>
    static auto runWithErrorHandling(
        const std::string& operationName,
        Func&& func,
        Args&&... args
    ) -> std::optional<std::invoke_result_t<Func, Args...>>;
    
    // Combined timeout and error handling
    template<typename Func, typename... Args>
    static auto runWithTimeoutAndErrorHandling(
        const std::string& operationName,
        std::chrono::milliseconds timeout,
        Func&& func,
        Args&&... args
    ) -> std::optional<std::invoke_result_t<Func, Args...>>;
};
```

## Redesigned ResourceHub

With these patterns in place, we can redesign ResourceHub to use composition:

```cpp
class ResourceHub {
private:
    // Composition over inheritance
    SafeGraph<std::shared_ptr<Resource>> resourceGraph_;
    ThreadPoolExecutor threadPool_;
    MemoryBudget<Resource> memoryBudget_;
    
public:
    // Clean, simplified API
    template<typename T>
    ResourceHandle<T> load(const std::string& typeId, const std::string& resourceId);
    
    template<typename T>
    void loadAsync(
        const std::string& typeId, 
        const std::string& resourceId,
        ResourcePriority priority,
        std::function<void(ResourceHandle<T>)> callback
    );
    
    // Dependency management
    bool addDependency(const std::string& dependentId, const std::string& dependencyId);
    bool removeDependency(const std::string& dependentId, const std::string& dependencyId);
    
    // Resource management
    bool unload(const std::string& resourceId, bool cascade = false);
    
    // Testing support
    void reset();
    bool isEmpty() const;
};
```

## Testing Improvements

With this architecture, testing becomes much simpler:

1. **Isolated Component Testing**: Each component (ThreadPoolExecutor, SafeGraph, etc.) can be tested in isolation
2. **Mock Components**: For testing ResourceHub, components can be mocked
3. **Deterministic Testing**: ThreadPoolExecutor has pauseForTesting() that disables actual threading
4. **RAII for Resources**: More reliable resource cleanup with RAII patterns
5. **Clear Error Boundaries**: Each component has clear error handling responsibility

## Next Steps

1. Implement the pattern library components
2. Refactor ResourceHub to use the new components
3. Update tests to use the new architecture
4. Document the patterns and provide usage examples
5. Create migration guide for existing code

This approach will significantly improve maintainability, testability, and reliability while reducing code duplication and complexity.