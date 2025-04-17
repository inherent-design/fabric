# Claude notes

## Tests

When fixing or running tests, make sure to disable worker threads in ResourceHub to prevent test hangs.

```cpp
// Disable worker threads in setup
ResourceHub::instance().disableWorkerThreadsForTesting();

// Check that they were disabled
ASSERT_EQ(ResourceHub::instance().getWorkerThreadCount(), 0);
```

For `ResourceHub` related tests, prefer:
1. Using direct `Resource` objects rather than going through the hub
2. Using explicit timeouts on all locks to prevent deadlocks
3. Handling exceptions with try/catch to ensure tests can clean up resources
4. Creating simpler, more focused tests that test one thing at a time

## Build

```bash
# Build
cmake -G "Ninja" -B build && cmake --build build

# Test
./build/bin/UnitTests  # or IntegrationTests, E2ETests
./build/bin/UnitTests --gtest_filter=TestName
```

## Code Style

- `.hh` for headers, `.cc` for implementations
- PascalCase for classes, camelCase for methods
- Use namespaces and forward declarations
- Use `std::unique_ptr` or `std::shared_ptr` for ownership management
- Prefer `const` where possible

## Error handling

- Use `throwError()` function not direct exception throws
- Use `Logger::log*()` methods for logging (logDebug, logInfo, logWarning, logError, logCritical)
- Catch and handle exceptions at appropriate boundaries

## Concurrency best practices

- Always use intent-based locking:
  - `Read` - Shared read access
  - `NodeModify` - Exclusive access to node data
  - `GraphStructure` - Exclusive access to graph structure

- Always use timeouts with locks to prevent deadlocks:
  ```cpp
  auto nodeLock = node->tryLock(
      CoordinatedGraph<std::shared_ptr<Resource>>::LockIntent::Read,
      50); // 50ms timeout
  
  if (!nodeLock || !nodeLock->isLocked()) {
    // Handle lock acquisition failure
    return;
  }
  
  // Use the resource
  auto resource = nodeLock->getNode()->getData();
  
  // Release the lock
  nodeLock->release();
  ```

- Use the Copy-Then-Process pattern from IMPLEMENTATION_PATTERNS.md:
  ```cpp
  // First acquire a lock and copy the data
  std::vector<KeyType> allResourceIds;
  {
    auto lock = resourceGraph_.lockGraph(LockIntent::Read, 100);
    if (!lock || !lock->isLocked()) {
      return false;
    }
    
    allResourceIds = resourceGraph_.getAllNodes();
  }
  
  // Then process the data without holding the lock
  for (const auto& id : allResourceIds) {
    // Process resources...
  }
  ```

## Breaking circular dependencies

- Forward declare in base class
- Use a separate helper class to implement functionality
- Use dependency injection

## Testing best practices

- Tests should be focused on one functionality
- Use proper setup and teardown
- Handle timeouts gracefully
- Avoid deadlocks by using the helper functions with timeouts
- Ensure all resources are cleaned up
- Avoid test dependencies on ResourceHub state