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
- Primary approach in Fabric's ConcurrentGraph implementation

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
class GraphResourceManager {
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

## Best Practices from Fabric

1. **Minimize Lock Duration**: Hold locks for the shortest time possible
2. **Use Appropriate Lock Types**: Read locks for reads, write locks for writes
3. **Avoid Nested Locks**: Prevent deadlocks by avoiding nested lock acquisition
4. **Consistent Lock Ordering**: When multiple locks are needed, always acquire in the same order
5. **Validate Inputs Before Locking**: Reduce time spent in critical sections
6. **Timeout All Operations**: Prevent infinite waits in production code
7. **Test Edge Cases**: Use randomized and stress testing for concurrency bugs

## Conclusion

Fabric Engine's concurrency architecture demonstrates that with careful design, complex resource management can be both thread-safe and high-performance. The fine-grained, node-level locking strategy with reader-writer separation provides an optimal balance between correctness and concurrency, making the most of modern multi-core processors while preventing race conditions and deadlocks.
