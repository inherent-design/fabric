# Concurrency Management in Fabric

This document explains the concurrency model used in the Fabric framework, focusing on how we manage access to shared resources and prevent race conditions.

## Overview

Fabric uses a sophisticated concurrency model based on intent declaration and awareness propagation, which allows for maximizing parallelism while preventing deadlocks. The core implementation is found in the `CoordinatedGraph` class, which serves as the foundation for resource management and other concurrent operations.

## Intent-Based Locking

The traditional approach to concurrency often relies on simple reader-writer locks, which can lead to deadlocks when complex operations need to acquire multiple locks. Fabric's intent-based locking system addresses this by:

1. Explicitly declaring the intent of each lock acquisition
2. Allowing operations to communicate their priority and purpose
3. Enabling coordination between different lock levels
4. Using proper timeout protection to prevent indefinite waits

Lock intents in the `CoordinatedGraph` are defined as:

- `Read`: For read-only operations (lowest priority)
- `NodeModify`: For operations that modify node data but not graph structure
- `GraphStructure`: For operations that modify the graph structure (highest priority)

## Awareness Propagation

When a high-priority operation (like a structural change) needs to occur, it doesn't immediately fail if lower-priority locks are held. Instead:

1. The operation signals its intent to the existing lock holders
2. Lock holders can react appropriately (by yielding, completing quickly, etc.)
3. The system intelligently manages lock acquisition to prevent starvation

This awareness propagation allows for a more cooperative concurrency model, where operations can communicate their needs to each other.

## Lock Hierarchy

To prevent deadlocks, Fabric enforces a strict lock hierarchy:

1. Graph-level locks are always acquired before node-level locks
2. Node-level locks are never upgraded to graph-level locks
3. When multiple locks are needed, they're acquired in a consistent order

The lock acquisition protocol follows this pattern:
```
Get a graph-level lock first → Then acquire node locks as needed → Release graph lock to prevent deadlocks
```

## Timeout Protection

All lock acquisition operations in Fabric have timeout protection to prevent indefinite waits. This ensures that even in highly contended scenarios, operations will eventually get a chance to execute or fail gracefully. Default timeouts are short (e.g., 100ms) to quickly detect contention issues.

## Resource Management

The `ResourceHub` class builds on the `CoordinatedGraph` to provide thread-safe resource management, including:

- Concurrent loading and unloading of resources
- Dependency tracking between resources
- Memory budget enforcement with graceful eviction
- Asynchronous resource loading

By leveraging the intent-based locking system, `ResourceHub` can perform complex operations like topological sorting and dependency traversal while maintaining thread safety.

## Best Practices

When working with concurrent operations in Fabric:

1. Always declare your lock intent explicitly
2. Use the smallest scope possible for locks
3. Avoid holding locks during I/O or long-running operations
4. Handle lock acquisition failures gracefully
5. Follow the established lock hierarchy
6. Use proper timeout protection on all lock acquisitions
7. Consider local copies of data to minimize lock duration

## Implementation Details

For the specific implementation details, refer to:

- `include/fabric/core/CoordinatedGraph.hh`: Intent-based graph concurrency
- `include/fabric/core/ResourceHub.hh`: Thread-safe resource management
- `src/core/ResourceHub.cc`: Implementation of resource coordination