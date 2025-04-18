# Testing Best Practices

This document outlines key patterns and practices for testing Fabric components, with special focus on thread-safe systems like ResourceHub.

## General Testing Principles

1. **Independence**: Each test must be fully independent and self-contained
2. **Focused Scope**: Test a single feature or behavior in each test case
3. **Thorough Cleanup**: Always clean up resources even when tests fail
4. **Timeout Protection**: Add timeouts to prevent tests from hanging
5. **Error Handling**: Properly catch and report exceptions

## Testing Thread-Safe Components

### Worker Thread Management

When testing components that use worker threads:

```cpp
// In setup
ResourceHub::instance().disableWorkerThreadsForTesting();

// Verify threads are disabled
ASSERT_EQ(ResourceHub::instance().getWorkerThreadCount(), 0);

// In teardown
ResourceHub::instance().reset();
```

### Lock Timeout Protection

Always use timeouts with locks to prevent deadlocks:

```cpp
// Get a node with timeout
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

### Copy-Then-Process Pattern

Minimize lock duration by copying data first:

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

### Test Timeout Mechanism

For complex tests that might hang:

```cpp
// Set timeout for test
const auto startTime = std::chrono::steady_clock::now();
const auto timeout = std::chrono::seconds(5);

// Run test code...

// Periodically check for timeout
if (std::chrono::steady_clock::now() - startTime > timeout) {
  FAIL() << "Test timed out after 5 seconds";
  return;
}
```

## ResourceHub-Specific Testing

### Prefer Direct Testing

When possible, test Resource objects directly:

```cpp
// Instead of using ResourceHub
auto resource = std::make_shared<TestResource>("resId");
ASSERT_TRUE(resource->load());

// Then test the resource directly
EXPECT_EQ(resource->getState(), ResourceState::Loaded);
EXPECT_GT(resource->getMemoryUsage(), 0);
```

### Use Test Resources

Create simple test resources for predictable behavior:

```cpp
class TestResource : public Resource {
public:
  explicit TestResource(const std::string& id) : Resource(id) {}
  
  size_t getMemoryUsage() const override { return memUsage_; }
  void setMemoryUsage(size_t bytes) { memUsage_ = bytes; }
  
protected:
  bool loadImpl() override { return true; }
  void unloadImpl() override {}
  
private:
  size_t memUsage_ = 1024;
};
```

### Testing Dependency Relationships

Test dependency relationships with controlled setup:

```cpp
// Add resources and dependencies
resourceHub.addResource("resA", std::make_shared<TestResource>("resA"));
resourceHub.addResource("resB", std::make_shared<TestResource>("resB"));
resourceHub.addDependency("resB", "resA");

// Verify the dependency exists
auto deps = resourceHub.getDependencies("resB");
EXPECT_EQ(deps.size(), 1);
EXPECT_TRUE(deps.count("resA"));

// Verify the dependent relationship
auto dependents = resourceHub.getDependents("resA");
EXPECT_EQ(dependents.size(), 1);
EXPECT_TRUE(dependents.count("resB"));
```

### Testing Concurrent Access

For testing concurrent access patterns:

```cpp
// Create a stress test with multiple threads
void runConcurrentTest(int threadCount, int operationsPerThread) {
  std::vector<std::thread> threads;
  std::atomic<int> errors{0};
  
  for (int i = 0; i < threadCount; ++i) {
    threads.push_back(std::thread([i, operationsPerThread, &errors]() {
      for (int j = 0; j < operationsPerThread; ++j) {
        try {
          // Perform operations on ResourceHub
          std::string resId = "res_" + std::to_string(i) + "_" + std::to_string(j);
          ResourceHub::instance().addResource(resId, 
                                            std::make_shared<TestResource>(resId));
        } catch (const std::exception& e) {
          errors++;
        }
      }
    }));
  }
  
  // Wait for all threads to complete
  for (auto& thread : threads) {
    thread.join();
  }
  
  EXPECT_EQ(errors, 0) << "Concurrent operations caused errors";
}
```

## Common Testing Pitfalls

### 1. Deadlocks in Tests

**Symptoms**:
- Tests hang indefinitely
- Timeouts in CI/CD pipelines

**Prevention**:
- Always use timeouts with locks
- Disable worker threads for testing
- Follow consistent lock ordering
- Use the Copy-Then-Process pattern

### 2. Order Dependencies

**Symptoms**:
- Tests pass/fail depending on run order
- Flaky tests in CI/CD

**Prevention**:
- Reset shared state before each test
- Ensure ResourceHub is empty at test start
- Avoid global state dependencies

### 3. Resource Leaks

**Symptoms**:
- Memory usage grows over time
- Error messages about resource cleanup

**Prevention**:
- Always clean up in tearDown
- Use RAII patterns for resources
- Add explicit unload/reset calls

## Conclusion

Effective testing of Fabric's components, especially ResourceHub, requires careful attention to concurrency, resource management, and independence. By following these best practices, you can create tests that are reliable, fast, and effective at catching regressions.