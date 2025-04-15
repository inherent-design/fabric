#include "fabric/core/ResourceHub.hh"
#include "fabric/utils/Testing.hh"
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <string>
#include <atomic>

namespace Fabric {
namespace Test {

using namespace Fabric;
using namespace Fabric::Testing;
using ::testing::_;
using ::testing::Return;

// Simple test resource class for minimal tests
class MinimalTestResource : public Resource {
public:
  explicit MinimalTestResource(const std::string& id, size_t memSize = 1024)
    : Resource(id), memorySize(memSize), loadCount(0), unloadCount(0) {}
  
  bool loadImpl() override {
    loadCount++;
    return true;
  }
  
  void unloadImpl() override {
    unloadCount++;
  }
  
  size_t getMemoryUsage() const override {
    return memorySize;
  }
  
  int getLoadCount() const { return loadCount; }
  int getUnloadCount() const { return unloadCount; }
  
private:
  size_t memorySize;
  std::atomic<int> loadCount{0};
  std::atomic<int> unloadCount{0};
};

// Minimal test class for resource hub with simplified setup
class ResourceHubMinimalTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Explicitly disable worker threads to avoid concurrency issues
    ResourceHub::instance().disableWorkerThreadsForTesting();
    
    // Clear any resources from previous tests
    ResourceHub::instance().clear();
    
    // Register factory for test resource
    if (!ResourceFactory::isTypeRegistered("TestResource")) {
      ResourceFactory::registerType<MinimalTestResource>(
        "TestResource", 
        [](const std::string& id) {
          return std::make_shared<MinimalTestResource>(id);
        }
      );
    }
  }
  
  void TearDown() override {
    // Clean up resources
    ResourceHub::instance().clear();
  }
};

// Test just creating a resource directly
TEST_F(ResourceHubMinimalTest, DirectResourceCreation) {
  auto resource = std::make_shared<MinimalTestResource>("test");
  EXPECT_EQ(resource->getId(), "test");
  EXPECT_EQ(resource->getState(), ResourceState::Unloaded);
}

// Test loading and unloading a resource directly
TEST_F(ResourceHubMinimalTest, DirectResourceLoadUnload) {
  auto resource = std::make_shared<MinimalTestResource>("test");
  
  // Load resource
  bool loaded = resource->load();
  EXPECT_TRUE(loaded);
  EXPECT_EQ(resource->getState(), ResourceState::Loaded);
  EXPECT_EQ(resource->getLoadCount(), 1);
  
  // Unload resource
  resource->unload();
  EXPECT_EQ(resource->getState(), ResourceState::Unloaded);
  EXPECT_EQ(resource->getUnloadCount(), 1);
}

// Test resource factory
TEST_F(ResourceHubMinimalTest, ResourceFactoryCreate) {
  auto resource = ResourceFactory::create("TestResource", "factoryTest");
  ASSERT_NE(resource, nullptr);
  EXPECT_EQ(resource->getId(), "factoryTest");
}

// Test very basic ResourceHub load without worker threads
TEST_F(ResourceHubMinimalTest, BasicResourceHubLoad) {
  auto& hub = ResourceHub::instance();
  
  // Load resource through hub
  auto handle = hub.load<MinimalTestResource>("TestResource", "hubTest");
  
  ASSERT_TRUE(handle);
  EXPECT_EQ(handle->getState(), ResourceState::Loaded);
  
  // Check if resource exists in hub
  EXPECT_TRUE(hub.hasResource("hubTest"));
  EXPECT_TRUE(hub.isLoaded("hubTest"));
}

// Test memory budget setting
TEST_F(ResourceHubMinimalTest, MemoryBudget) {
  auto& hub = ResourceHub::instance();
  
  const size_t testBudget = 1024 * 1024; // 1 MB
  hub.setMemoryBudget(testBudget);
  
  EXPECT_EQ(hub.getMemoryBudget(), testBudget);
}

// Test very basic dependency creation
TEST_F(ResourceHubMinimalTest, BasicDependency) {
  auto& hub = ResourceHub::instance();
  
  // Create two resources
  hub.load<MinimalTestResource>("TestResource", "dep1");
  hub.load<MinimalTestResource>("TestResource", "dep2");
  
  // Add dependency: dep1 -> dep2
  EXPECT_TRUE(hub.addDependency("dep1", "dep2"));
  
  // Check dependency exists
  auto dependencies = hub.getDependencyResources("dep1");
  ASSERT_EQ(dependencies.size(), 1);
  EXPECT_EQ(dependencies[0], "dep2");
  
  // Check dependent relationship exists
  auto dependents = hub.getDependentResources("dep2");
  ASSERT_EQ(dependents.size(), 1);
  EXPECT_EQ(dependents[0], "dep1");
}

} // namespace Test
} // namespace Fabric