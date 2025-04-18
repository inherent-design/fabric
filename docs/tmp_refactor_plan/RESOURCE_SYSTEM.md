# Resource Management System

## Overview

The resource management system in Fabric follows a modular design with clear separation of concerns. It provides efficient loading, unloading, dependency tracking, and memory management for all game resources.

## Key Components

### ResourceHub

The central facade that coordinates all resource management operations:

```cpp
class ResourceHub {
public:
  static ResourceHub &instance();
  
  // Synchronous loading
  template <typename T>
  ResourceHandle<T> load(const std::string &typeId, const std::string &resourceId);
  
  // Asynchronous loading
  template <typename T>
  void loadAsync(const std::string &typeId, const std::string &resourceId,
                ResourcePriority priority,
                std::function<void(ResourceHandle<T>)> callback);
  
  // Dependency management
  bool addDependency(const std::string &dependentId, const std::string &dependencyId);
  bool removeDependency(const std::string &dependentId, const std::string &dependencyId);
  
  // Unloading
  bool unload(const std::string &resourceId);
  bool unload(const std::string &resourceId, bool cascade);
  bool unloadRecursive(const std::string &resourceId);
  
  // Memory management
  void setMemoryBudget(size_t bytes);
  size_t getMemoryBudget() const;
  size_t getMemoryUsage() const;
  size_t enforceMemoryBudget();
  
  // Thread management
  void setWorkerThreadCount(unsigned int count);
  unsigned int getWorkerThreadCount() const;
  
  // Query methods
  std::unordered_set<std::string> getDependents(const std::string &resourceId);
  std::unordered_set<std::string> getDependencies(const std::string &resourceId);
  bool hasResource(const std::string &resourceId);
  bool isLoaded(const std::string &resourceId) const;
  
  // Lifecycle
  void clear();
  void reset();
  void shutdown();
  
private:
  // Component managers
  std::shared_ptr<ResourceLoader> resourceLoader_;
  std::shared_ptr<ResourceDependencyManager> dependencyManager_;
  std::shared_ptr<ResourceMemoryManager> memoryManager_;
  std::shared_ptr<ResourceThreadPool> threadPool_;
};
```

### ResourceLoader

Focused on loading resources from different sources:

```cpp
class ResourceLoader {
public:
  ResourceLoader(
    std::shared_ptr<ResourceThreadPool> threadPool,
    std::shared_ptr<ResourceDependencyManager> dependencyManager);

  template <typename T>
  ResourceHandle<T> load(const std::string &typeId, const std::string &resourceId);

  template <typename T>
  void loadAsync(
    const std::string &typeId,
    const std::string &resourceId,
    ResourcePriority priority,
    std::function<void(ResourceHandle<T>)> callback);

  void preload(
    const std::vector<std::string> &typeIds,
    const std::vector<std::string> &resourceIds,
    ResourcePriority priority = ResourcePriority::Low);

private:
  std::shared_ptr<ResourceThreadPool> threadPool_;
  std::shared_ptr<ResourceDependencyManager> dependencyManager_;
};
```

### ResourceDependencyManager

Tracks dependencies between resources using a directed graph:

```cpp
class ResourceDependencyManager {
public:
  bool addDependency(const std::string &dependentId, const std::string &dependencyId);
  bool removeDependency(const std::string &dependentId, const std::string &dependencyId);
  bool hasResource(const std::string &resourceId);
  bool addResource(const std::string &resourceId, std::shared_ptr<Resource> resource);
  bool removeResource(const std::string &resourceId, bool cascade = false);
  std::unordered_set<std::string> getDependents(const std::string &resourceId);
  std::unordered_set<std::string> getDependencies(const std::string &resourceId);
  void clear();
  
private:
  CoordinatedGraph<std::shared_ptr<Resource>> resourceGraph_;
};
```

### ResourceMemoryManager

Enforces memory budgets and handles eviction:

```cpp
class ResourceMemoryManager {
public:
  explicit ResourceMemoryManager(std::shared_ptr<ResourceDependencyManager> dependencyManager);
  void setMemoryBudget(size_t bytes);
  size_t getMemoryBudget() const;
  size_t getMemoryUsage() const;
  size_t enforceMemoryBudget();
  bool registerResource(const std::string &resourceId);
  bool unregisterResource(const std::string &resourceId);
  
private:
  std::atomic<size_t> memoryBudget_;
  std::shared_ptr<ResourceDependencyManager> dependencyManager_;
};
```

### ResourceThreadPool

Manages worker threads for asynchronous loading:

```cpp
class ResourceThreadPool {
public:
  explicit ResourceThreadPool(unsigned int threadCount = std::thread::hardware_concurrency());
  void queueRequest(const ResourceLoadRequest &request);
  unsigned int getWorkerThreadCount() const;
  void setWorkerThreadCount(unsigned int count);
  void shutdown();
  
private:
  void workerThreadFunc();
  std::atomic<unsigned int> workerThreadCount_;
  std::vector<std::unique_ptr<std::thread>> workerThreads_;
  ThreadSafeQueue<ResourceLoadRequest> requestQueue_;
};
```

### ThreadSafeQueue

Thread-safe container for resource requests:

```cpp
template <typename T>
class ThreadSafeQueue {
public:
  void push(const T& item);
  bool tryPop(T& item);
  
  template <typename Predicate>
  bool waitAndPop(T& item, Predicate predicate);
  
  template <typename Predicate>
  bool waitAndPopWithTimeout(T& item, int timeoutMs, Predicate predicate);
  
  bool empty() const;
  size_t size() const;
  void clear();
  
private:
  mutable std::timed_mutex mutex_;
  std::condition_variable_any condition_;
  std::queue<T> queue_;
};
```

## Resource Base Class

All resources share a common interface:

```cpp
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
```

## Resource Lifecycle

Resources move through these states during their lifecycle:

1. **Requested**: ResourceHub.load() has been called
2. **Loading**: ResourceLoader is actively loading the resource
3. **Loaded**: Resource data is available in memory
4. **Active/Inactive**: Resource is actively used or not currently referenced
5. **Eviction**: Resource may be unloaded if memory budget is exceeded
6. **Unloaded**: Resource has been removed from memory

## Memory Management

The memory manager enforces budgets through several mechanisms:

1. **Memory Tracking**: Each resource reports its memory usage
2. **Budget Enforcement**: Total memory usage is kept below the budget
3. **LRU Eviction**: Least recently used resources are evicted first
4. **Dependency Awareness**: Resources with active dependents are protected

## Asynchronous Loading

Resources can be loaded asynchronously with priority levels:

1. **Priorities**: Highest, High, Normal, Low, Lowest
2. **Thread Pool**: Worker threads process requests according to priority
3. **Callbacks**: Notification when resources are loaded
4. **Batch Loading**: Multiple resources can be preloaded in a batch