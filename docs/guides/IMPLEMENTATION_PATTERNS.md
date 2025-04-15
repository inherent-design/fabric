# Implementation Patterns for Concurrent Graph Systems

## Introduction

This document provides practical implementation patterns for working with Fabric's concurrent graph-based systems. These patterns represent battle-tested approaches that balance correctness, performance, and code clarity.

## Node Design Patterns

### The Node Wrapper Pattern

When implementing graph nodes, wrap your data with thread-safe access methods:

```cpp
template <typename T>
class ThreadSafeNode {
private:
    T data_;
    mutable std::shared_mutex mutex_;
    std::chrono::steady_clock::time_point lastAccessTime_;

public:
    // Thread-safe read access
    const T& getData() const {
        std::shared_lock lock(mutex_);
        return data_;
    }

    // Thread-safe write access
    void updateData(std::function<void(T&)> updateFunc) {
        std::unique_lock lock(mutex_);
        updateFunc(data_);
        lastAccessTime_ = std::chrono::steady_clock::now();
    }
};
```

#### When to Use

- When node data may be accessed from multiple threads
- When tracking access time is important (e.g., for LRU policies)
- When you need different access patterns for reading vs. writing

### The Immutable Node Pattern

For high-throughput scenarios, consider immutable nodes with atomically-swapped references:

```cpp
template <typename T>
class ImmutableNode {
private:
    std::atomic<std::shared_ptr<const T>> data_;
    
public:
    // Thread-safe read access
    std::shared_ptr<const T> getData() const {
        return std::atomic_load(&data_);
    }
    
    // Thread-safe update
    void setData(std::shared_ptr<const T> newData) {
        std::atomic_store(&data_, newData);
    }
};
```

#### When to Use

- When updates are infrequent compared to reads
- For lock-free access patterns in performance-critical code
- When data structures are large and copying would be expensive

## Traversal Patterns

### Copy-Then-Process

To minimize lock durations during traversal:

```cpp
// Bad: Holding lock during processing
void processNodes() {
    std::lock_guard lock(graphMutex_);
    for (const auto& [key, node] : nodes_) {
        heavyOperation(node); // Long processing under lock!
    }
}

// Good: Copy, then process without lock
void processNodes() {
    std::vector<std::shared_ptr<Node>> nodesCopy;
    {
        std::lock_guard lock(graphMutex_);
        nodesCopy.reserve(nodes_.size());
        for (const auto& [key, node] : nodes_) {
            nodesCopy.push_back(node);
        }
    }
    // Process outside the lock
    for (const auto& node : nodesCopy) {
        heavyOperation(node);
    }
}
```

#### When to Use

- When traversal must see a consistent snapshot of the graph
- When processing operations are expensive or may block
- When minimum lock contention is critical

### Visitor Pattern with Early Lock Release

For operations that must be applied to each node:

```cpp
void visitNodes(std::function<void(const Key&, Node&)> visitor) {
    for (const auto& key : getAllNodeKeys()) {
        auto node = getNode(key);
        if (!node) continue; // Node might have been removed
        
        auto lock = node->lockExclusive();
        try {
            visitor(key, *node);
        } catch (...) {
            // Ensure lock is released even on exception
            lock.unlock();
            throw;
        }
    }
}
```

#### When to Use

- When applying operations to many nodes
- When operations might throw exceptions
- When fine-grained, per-node locking is appropriate

## Dependency Management Patterns

### Topological Processing

When operations must respect dependencies:

```cpp
bool processDependencyOrder(std::function<void(const Key&, Node&)> processFunc) {
    // Get topologically sorted order
    auto sorted = topologicalSort();
    if (sorted.empty() && !nodes_.empty()) {
        return false; // Cycle detected
    }
    
    // Process in dependency order
    for (const auto& key : sorted) {
        auto node = getNode(key);
        if (node) {
            auto lock = node->lockExclusive();
            processFunc(key, *node);
        }
    }
    return true;
}
```

#### When to Use

- When processing order matters (dependencies before dependents)
- For initialization and shutdown sequences
- For resource loading and unloading operations

### Dependency-Aware Eviction

When removing nodes, consider their dependencies:

```cpp
bool canSafelyEvict(const Key& key) {
    // Check if node exists
    if (!hasNode(key)) return false;
    
    // Check for dependents
    auto dependents = getInEdges(key);
    if (!dependents.empty()) return false;
    
    // Check for external references
    auto node = getNode(key);
    if (!node || node.use_count() > 1) return false;
    
    return true;
}
```

#### When to Use

- In memory management systems with dependency relationships
- When implementing LRU caches for resources
- When safely unloading components from a running system

## Concurrency Control Patterns

### Multi-Phase Locking

For operations that need consistent access to multiple nodes:

```cpp
bool transferBetweenNodes(const Key& from, const Key& to, T data) {
    // Phase 1: Verify nodes exist
    auto fromNode = getNode(from);
    auto toNode = getNode(to);
    if (!fromNode || !toNode) return false;
    
    // Phase 2: Lock nodes in consistent order to prevent deadlock
    // (always lock in key order)
    std::unique_lock<std::shared_mutex> lock1(from < to ? 
                                             fromNode->mutex_ : 
                                             toNode->mutex_);
    std::unique_lock<std::shared_mutex> lock2(from < to ? 
                                             toNode->mutex_ : 
                                             fromNode->mutex_);
    
    // Phase 3: Verify conditions still hold after locking
    if (!fromNode->hasData(data)) return false;
    
    // Phase 4: Perform the operation
    fromNode->removeData(data);
    toNode->addData(data);
    return true;
}
```

#### When to Use

- When multiple nodes must be updated atomically
- To avoid deadlocks when acquiring multiple locks
- When state verification must happen after locking

### Read-Modify-Write

For updating node data based on its current state:

```cpp
template <typename T, typename Func>
T updateNodeValue(const Key& key, Func updateFunc) {
    auto node = getNode(key);
    if (!node) throw std::runtime_error("Node not found");
    
    auto lock = node->lockExclusive();
    T currentValue = node->getValue();
    T newValue = updateFunc(currentValue);
    node->setValue(newValue);
    return newValue;
}
```

#### When to Use

- When updates depend on current values
- For thread-safe counter increments or state transitions
- When atomic read-modify-write semantics are needed

## Advanced Patterns

### Work Stealing for Graph Operations

For parallelizing graph operations across multiple cores:

```cpp
void parallelTraverse(const Key& startKey) {
    WorkQueue queue;
    queue.push(startKey);
    
    // Create worker threads
    std::vector<std::thread> workers;
    for (unsigned i = 0; i < std::thread::hardware_concurrency(); ++i) {
        workers.push_back(std::thread([&queue, this]() {
            Key key;
            while (queue.try_pop(key)) {
                processNode(key);
                
                // Add neighbors to work queue
                auto outEdges = getOutEdges(key);
                for (const auto& neighbor : outEdges) {
                    if (shouldVisit(neighbor)) {
                        queue.push(neighbor);
                    }
                }
            }
        }));
    }
    
    // Wait for completion
    for (auto& worker : workers) {
        worker.join();
    }
}
```

#### When to Use

- For CPU-intensive graph operations
- When traversals don't have strict ordering requirements
- For large graphs where parallelism provides significant benefits

### Node Batching for Bulk Operations

For efficient bulk operations on multiple nodes:

```cpp
template <typename T>
void batchUpdate(const std::vector<Key>& keys, const T& value) {
    // Pre-allocate results
    std::vector<bool> results(keys.size(), false);
    
    // Process in batches to avoid excessive lock contention
    const size_t BATCH_SIZE = 100;
    
    for (size_t i = 0; i < keys.size(); i += BATCH_SIZE) {
        // Process a batch concurrently
        size_t end = std::min(i + BATCH_SIZE, keys.size());
        RunConcurrent(end - i, 1, [&](size_t threadIdx, size_t) {
            size_t idx = i + threadIdx;
            auto node = getNode(keys[idx]);
            if (node) {
                auto lock = node->lockExclusive();
                node->setValue(value);
                results[idx] = true;
            }
        });
    }
}
```

#### When to Use

- When operating on many nodes at once
- To avoid thread oversubscription by controlling batch size
- For operations where results can be processed in any order

## Testing Patterns

### Deterministic Testing

For reliable unit testing of concurrent code:

```cpp
TEST_F(GraphTest, ConcurrentAccess) {
    // Setup test graph with known state
    graph.addNode("A", TestData(1));
    graph.addNode("B", TestData(2));
    graph.addEdge("A", "B");
    
    // Run the test with timeout protection
    bool success = RunWithTimeout([&]() {
        RunConcurrent(4, 100, [&](size_t threadId, size_t iteration) {
            // Use different operations based on thread ID
            switch (threadId % 4) {
                case 0: graph.getNode("A"); break;
                case 1: graph.getNode("B"); break;
                case 2: graph.hasEdge("A", "B"); break;
                case 3: graph.getInEdges("B"); break;
            }
        });
    }, std::chrono::milliseconds(5000));
    
    ASSERT_TRUE(success) << "Test timed out";
    
    // Verify graph remains consistent
    EXPECT_TRUE(graph.hasNode("A"));
    EXPECT_TRUE(graph.hasNode("B"));
    EXPECT_TRUE(graph.hasEdge("A", "B"));
}
```

#### When to Use

- For unit testing concurrent graph operations
- When tests must be deterministic and reliable
- To verify that timeouts don't occur in normal operation

## Conclusion

These implementation patterns form a toolkit for working with Fabric's graph-based systems. By applying these patterns appropriately, you can create code that is:

1. **Correct**: Properly handles concurrent access without race conditions
2. **Efficient**: Minimizes lock contention and maximizes parallel execution
3. **Readable**: Follows established patterns that are easier to understand
4. **Maintainable**: Structured in ways that make bugs less likely

Remember that no single pattern fits all situations. The choice of pattern should be guided by the specific requirements of your use case, particularly considering the frequency of reads vs. writes, the complexity of operations, and the performance characteristics needed.