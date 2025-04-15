# Concurrency in Fabric Engine

## Introduction

Effective concurrency is vital in modern applications, particularly in resource-intensive systems like Fabric Engine. This document outlines the concurrency patterns and implementations that enable Fabric to efficiently utilize modern hardware.

## Concurrency Fundamentals in Resource Management

### Concurrency vs. Parallelism

- **Concurrency**: Managing multiple logical tasks with overlapping lifetimes
- **Parallelism**: Physically executing multiple tasks simultaneously

In Von Neumann architecture systems with limited cores, effective concurrency becomes more important than raw parallelism. Fabric is designed with this principle in mind.

### Core Concurrency Challenges

1. **Race Conditions**: Multiple threads accessing shared data with unpredictable ordering
2. **Deadlocks**: Circular waiting for resources between threads
3. **Resource Starvation**: Some threads never getting access to resources
4. **Thread Safety**: Ensuring operations remain correct under concurrent access
5. **Performance Degradation**: Excessive synchronization reducing throughput

## Modern Locking Strategies

### The Granularity Spectrum

**Coarse-Grained Locking**
- Locks large sections of data (e.g., entire resource collection)
- Simple but creates high contention and reduces parallelism
- Used in Fabric only for operations that logically must be atomic

**Fine-Grained Locking**
- Locks smaller units (individual resources/nodes)
- Reduces contention but increases complexity
- Primary approach in Fabric's CoordinatedGraph implementation

### Reader-Writer Separation

Fabric uses C++17's `std::shared_mutex` extensively:
```cpp
mutable std::shared_mutex mutex_;

// For readers (multiple concurrent allowed)
std::shared_lock<std::shared_mutex> lock(mutex_);

// For writers (exclusive access)
std::unique_lock<std::shared_mutex> lock(mutex_);
```

This pattern optimizes for the common case of many reads with few writes, providing significant performance benefits for read-heavy workloads.

## Fabric's Thread Safety Implementation

### Node-Level Locking

Each resource node has its own mutex, allowing maximum parallelism:

```cpp
class Node {
public:
    std::shared_lock<std::shared_mutex> lockShared() {
        return std::shared_lock<std::shared_mutex>(mutex_);
    }

    std::unique_lock<std::shared_mutex> lockExclusive() {
        return std::unique_lock<std::shared_mutex>(mutex_);
    }

private:
    mutable std::shared_mutex mutex_;
};
```

### Copy-on-Read Pattern

Fabric implements a copy-then-process pattern to minimize lock duration:

```cpp
// Example from BFS implementation
std::unordered_set<KeyType> outEdgesForNode;
{
    std::shared_lock<std::shared_mutex> lock(graphMutex_);
    auto edgeIt = outEdges_.find(currentKey);
    if (edgeIt != outEdges_.end()) {
        outEdgesForNode = edgeIt->second; // Make a copy while locked
    }
}
// Process outEdgesForNode without holding the lock
```

### Exception-Safe Locking

All operations ensure locks are released even during exceptions:

```cpp
auto nodeLock = node->lockShared();
try {
    visitFunc(key, node->getData());
} catch (...) {
    // Ensure unlock happens if visitFunc throws
    nodeLock.unlock();
    throw;
}
```

## Worker Thread Management

### Thread Pool Implementation

Fabric's resource management employs a configurable thread pool:

```cpp
class ResourceHub {
private:
    std::atomic<unsigned int> workerThreadCount_;
    std::vector<std::unique_ptr<std::thread>> workerThreads_;
    std::priority_queue<ResourceLoadRequest> loadQueue_;
    std::mutex queueMutex_;
    std::condition_variable queueCondition_;
    std::atomic<bool> shutdown_{false};
};
```

### Efficient Thread Waiting

Worker threads wait efficiently using condition variables:

```cpp
std::unique_lock<std::mutex> lock(queueMutex_);
queueCondition_.wait(lock, [this] {
    return !loadQueue_.empty() || shutdown_;
});
```

### Hardware-Aware Scaling

Thread count automatically adjusts to available hardware:

```cpp
workerThreadCount_ = std::thread::hardware_concurrency();
```

## Memory Model Considerations

### Memory Ordering and Visibility

Fabric respects C++ memory ordering guarantees:

- **Atomic variables** for flags and counters
- **Memory barriers** with mutex operations
- **Acquire/release semantics** where appropriate

### Cache-Friendly Data Structures

- **Node clustering** for related resources
- **Memory alignment** for atomics and highly accessed fields
- **Contiguous containers** where possible

## Debugging and Testing Concurrent Code

### Timeout Protection for Tests

```cpp
template<typename Func>
bool RunWithTimeout(Func&& func, std::chrono::milliseconds timeout) {
    std::atomic<bool> completed{false};
    std::thread t([&]() {
        func();
        completed = true;
    });

    // Wait for completion or timeout
    auto start = std::chrono::steady_clock::now();
    while (!completed) {
        if (std::chrono::steady_clock::now() - start > timeout) {
            t.detach();
            return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    t.join();
    return true;
}
```

### Concurrent Stress Testing

```cpp
inline void RunConcurrent(
    size_t threadCount,
    size_t iterationsPerThread,
    std::function<void(size_t threadId, size_t iteration)> func)
{
    std::vector<std::future<void>> futures;
    futures.reserve(threadCount);

    for (size_t threadId = 0; threadId < threadCount; ++threadId) {
        futures.push_back(std::async(std::launch::async, [=]() {
            for (size_t i = 0; i < iterationsPerThread; ++i) {
                func(threadId, i);
            }
        }));
    }

    // Wait for all threads to complete
    for (auto& future : futures) {
        future.get();
    }
}
```

## Deadlock Prevention Techniques

### Timeout-Protected Locking

All lock acquisitions in Fabric now use timeout protection to prevent indefinite waiting:

```cpp
std::shared_lock<std::shared_mutex> lockShared(size_t timeoutMs = 100) {
    // First try a non-blocking lock attempt
    std::shared_lock<std::shared_mutex> lock(mutex_, std::try_to_lock);
    if (!lock.owns_lock()) {
        // If immediate locking failed, try with a timeout
        auto start = std::chrono::steady_clock::now();
        while (!lock.owns_lock()) {
            // Try to get the lock
            lock = std::shared_lock<std::shared_mutex>(mutex_, std::try_to_lock);
            if (lock.owns_lock()) {
                break;
            }
            
            // Check if we've exceeded the timeout
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start).count();
            if (elapsed >= timeoutMs) {
                // Return a non-owning lock if we timed out
                return std::shared_lock<std::shared_mutex>(mutex_, std::defer_lock);
            }
            
            // Short sleep to avoid CPU spinning
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
    return lock;
}
```

This approach ensures that operations will either succeed within a reasonable time or fail gracefully instead of hanging indefinitely.

### Lock Acquisition With Ownership Checks

All operations that acquire locks must check for ownership before proceeding:

```cpp
bool safeNodeOperation(const KeyType& key) {
    auto node = getNode(key, 50); // 50ms timeout
    if (!node) {
        // Node doesn't exist or couldn't acquire graph lock
        return false;
    }
    
    auto nodeLock = node->lockExclusive(50); // 50ms timeout
    if (!nodeLock.owns_lock()) {
        // Failed to acquire node lock within timeout
        return false;
    }
    
    // Safe to proceed with operations on the node
    // ...
    return true;
}
```

### Early Lock Release

Operations that need to check multiple resources release locks as soon as possible:

```cpp
void enforceMemoryBudget() {
    for (const auto& id : resourceIds) {
        auto node = resourceGraph_.getNode(id, 50);
        if (!node) continue;
        
        // First phase: check node status with shared lock
        {
            auto nodeLock = node->lockShared(50);
            if (!nodeLock.owns_lock()) continue;
            
            auto resource = node->getData();
            auto resourceSize = resource->getMemoryUsage();
            bool isLoaded = resource->getState() == ResourceState::Loaded;
            
            // Early release of lock before checking dependencies
            nodeLock.unlock();
            
            if (!isLoaded) continue;
        }
        
        // Check dependencies without holding node lock
        bool hasDependents = !resourceGraph_.getInEdges(id).empty();
        if (hasDependents) continue;
        
        // If eligible for eviction, re-acquire exclusive lock
        auto nodeLock = node->lockExclusive(50);
        if (!nodeLock.owns_lock()) continue;
        
        // Now safe to modify the node
        // ...
    }
}
```

### Test Design to Prevent Deadlocks

Tests are designed to avoid resource contention by using isolated resources:

```cpp
TEST_F(CoordinatedGraphTest, ConcurrentNodeAccess) {
    // Thread 0 works with nodes 0-99, Thread 1 with nodes 100-199, etc.
    for (int threadId = 0; threadId < THREAD_COUNT; ++threadId) {
        for (int i = 0; i < NODES_PER_THREAD; ++i) {
            int nodeId = threadId * NODES_PER_THREAD + i;
            std::string key = "thread_" + std::to_string(threadId) + "_node_" + std::to_string(i);
            graph.addNode(key, TestData("Node for thread " + std::to_string(threadId), nodeId));
        }
    }
    
    // Each thread only accesses its own nodes - no contention possible
    RunConcurrent(THREAD_COUNT, ITERATIONS_PER_THREAD, 
        [this, NODES_PER_THREAD](size_t threadId, size_t iteration) {
            // Each thread only accesses its own dedicated range of nodes
            int localNodeIdx = iteration % NODES_PER_THREAD;
            std::string key = "thread_" + std::to_string(threadId) + "_node_" + std::to_string(localNodeIdx);
            
            auto node = graph.getNode(key, LockIntent::Read, 10); // Read intent with short timeout
            if (node) {
                if (node.ownsLock()) {
                    // Process node...
                }
            }
        });
}
```

## Best Practices from Fabric

1. **Never Block Indefinitely**: Always use timeouts for lock acquisitions
2. **Minimize Lock Duration**: Hold locks for the shortest time possible
3. **Use Appropriate Lock Types**: Read locks for reads, write locks for writes
4. **Consistency in Lock Ordering**: When multiple locks are needed, always acquire in the same order
5. **Check Lock Ownership**: Always verify if lock was successfully acquired
6. **Release Locks Before Lengthy Operations**: Especially operations that might acquire other locks
7. **Design Tests for Non-Contention**: Use isolated resources in concurrent tests
8. **Timeout Protection for All Tests**: Prevent test hangs with timeout mechanisms

## Conclusion

Fabric Engine's concurrency architecture demonstrates that with careful design, complex resource management can be both thread-safe and high-performance. The fine-grained, node-level locking strategy with timeout protection and proper lock hierarchy provides a robust system that prevents deadlocks while maintaining high throughput. By consistently following these patterns throughout the codebase, Fabric achieves reliable concurrency even in complex scenarios with multiple interacting components.
