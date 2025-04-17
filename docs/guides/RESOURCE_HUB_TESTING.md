# ResourceHub Testing Guide

## Best Practices for Testing ResourceHub

ResourceHub is a complex, stateful singleton that manages resources with thread safety concerns. Testing it requires special care to avoid test hangs, deadlocks, and flaky tests.

### Setup and Teardown

Always properly initialize and clean up ResourceHub in tests:

```cpp
// In SetUp:
ResourceHub::instance().reset();  // Reset to a clean state
ASSERT_TRUE(ResourceHub::instance().isEmpty());
ASSERT_EQ(ResourceHub::instance().getWorkerThreadCount(), 0);

// In TearDown:
try {
  ResourceHub::instance().reset();
} catch (const std::exception& e) {
  std::cerr << "Error during teardown: " << e.what() << std::endl;
}
```

### Prefer Direct Testing

When possible, test Resource objects directly rather than going through the ResourceHub:

```cpp
// INSTEAD OF:
auto handle = ResourceHub::instance().load<TestResource>("TestType", "resId");

// PREFER:
auto resource = std::make_shared<TestResource>("resId");
ASSERT_TRUE(resource->load());
ResourceHandle<TestResource> handle(resource, nullptr);
```

### Use Timeouts and Error Handling

Always use explicit timeouts with locks and handle errors:

```cpp
// Get a node with timeout
auto node = resourceGraph_.getNode(resourceId, 100); // 100ms timeout
if (!node) {
  // Handle failure
  return;
}

// Lock with timeout
auto nodeLock = node->tryLock(
    CoordinatedGraph<std::shared_ptr<Resource>>::LockIntent::Read,
    100); // 100ms timeout
    
if (!nodeLock || !nodeLock->isLocked()) {
  // Handle lock failure
  return;
}

// Use RAII to ensure the lock is released
try {
  // Use the resource
  auto resource = nodeLock->getNode()->getData();
  // ...
} catch (const std::exception& e) {
  // Handle errors
} finally {
  // Release the lock
  nodeLock->release();
}
```

### Handle Thread Management

ResourceHub starts worker threads by default. Disable them in tests:

```cpp
// Disable threads at start of test
ResourceHub::instance().disableWorkerThreadsForTesting();

// Verify threads are disabled
ASSERT_EQ(ResourceHub::instance().getWorkerThreadCount(), 0);
```

### Use Test Timeouts

For tests that might hang, use a timeout mechanism:

```cpp
// Set up timeout mechanism
std::atomic<bool> testCompleted{false};
auto timeoutThread = std::thread([&testCompleted]() {
  // Wait for up to 3 seconds
  for (int i = 0; i < 30; ++i) {
    if (testCompleted) return;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  
  if (!testCompleted) {
    ADD_FAILURE() << "Test timeout detected - possible hang";
  }
});

// Run test...

// Mark test completed and clean up
testCompleted = true;
if (timeoutThread.joinable()) {
  timeoutThread.join();
}
```

### Create Focused Tests

Break complex tests into smaller, focused tests that each test one aspect:

```cpp
// INSTEAD OF:
TEST_F(ResourceHubTest, ComplexResourceWorkflow) {
  // 100+ lines testing everything at once
}

// PREFER:
TEST_F(ResourceHubTest, ResourceCreation) { /* Test creation */ }
TEST_F(ResourceHubTest, ResourceLoading) { /* Test loading */ }
TEST_F(ResourceHubTest, ResourceDependencies) { /* Test dependencies */ }
```

### Tests Should Be Independent

Ensure each test is independent of the others and ResourceHub's global state:

```cpp
// Reset at the start of every test
void SetUp() override {
  ResourceHub::instance().reset();
  ASSERT_TRUE(ResourceHub::instance().isEmpty());
}
```

## Common Issues and Solutions

### Deadlocks
- Always use timed locking operations
- Follow proper lock ordering (graph lock before node locks)
- Release locks immediately after use
- Use the Copy-Then-Process pattern

### Test Hangs
- Add timeouts to all operations
- Add test timeout mechanisms
- Disable worker threads
- Check for circular dependencies

### Flaky Tests
- Eliminate dependencies on ResourceHub's state
- Use direct Resource objects when possible
- Add proper error handling
- Ensure thorough cleanup