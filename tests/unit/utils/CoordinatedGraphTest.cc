#include "fabric/utils/CoordinatedGraph.hh"
#include "fabric/utils/Testing.hh"
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <mutex>

namespace Fabric {
namespace Test {

// Simple test class for CoordinatedGraph with minimal dependencies
class CoordinatedGraphMinimalTest : public ::testing::Test {
protected:
  struct TestData {
    std::string name;
    int value;

    TestData(std::string name = "", int value = 0)
        : name(std::move(name)), value(value) {}
  };

  using TestGraph = CoordinatedGraph<TestData>;

  void SetUp() override {
    graph = std::make_unique<TestGraph>();
  }

  void TearDown() override {
    graph.reset();
  }

  std::unique_ptr<TestGraph> graph;
};

// Test only the most basic functionality to isolate issues
TEST_F(CoordinatedGraphMinimalTest, BasicFunctionality) {
  // Add a node
  EXPECT_TRUE(graph->addNode("test", TestData("Test", 42)));
  
  // Verify node exists
  EXPECT_TRUE(graph->hasNode("test"));
  
  // Get node
  auto node = graph->getNode("test");
  ASSERT_NE(node, nullptr);
  
  // Get data with read lock
  {
    auto lock = node->tryLock(TestGraph::LockIntent::Read);
    ASSERT_TRUE(lock && lock->isLocked());
    EXPECT_EQ(node->getData().name, "Test");
    EXPECT_EQ(node->getData().value, 42);
  }
  
  // Remove node
  EXPECT_TRUE(graph->removeNode("test"));
  EXPECT_FALSE(graph->hasNode("test"));
}

// Test that we can lock and unlock nodes
TEST_F(CoordinatedGraphMinimalTest, LockingFunctionality) {
  // Create a node
  EXPECT_TRUE(graph->addNode("lockTest", TestData("Lock Test", 1)));
  
  // Get node
  auto node = graph->getNode("lockTest");
  ASSERT_NE(node, nullptr);
  
  // Test read lock acquisition and release
  {
    auto readLock = node->tryLock(TestGraph::LockIntent::Read);
    ASSERT_TRUE(readLock && readLock->isLocked());
  }
  
  // Test write lock acquisition and release
  {
    auto writeLock = node->tryLock(TestGraph::LockIntent::NodeModify);
    ASSERT_TRUE(writeLock && writeLock->isLocked());
    
    // Modify data
    node->getData().value = 2;
  }
  
  // Verify data was modified
  {
    auto readLock = node->tryLock(TestGraph::LockIntent::Read);
    ASSERT_TRUE(readLock && readLock->isLocked());
    EXPECT_EQ(node->getData().value, 2);
  }
}

// Test graph-level locking
TEST_F(CoordinatedGraphMinimalTest, GraphLevelLocking) {
  // Acquire a graph-level lock
  auto graphLock = graph->lockGraph(TestGraph::LockIntent::GraphStructure);
  ASSERT_TRUE(graphLock && graphLock->isLocked());
  
  // Should be able to modify graph while holding structure lock
  EXPECT_TRUE(graph->addNode("graphLockTest", TestData("Graph Lock Test", 3)));
  
  // Release lock
  graphLock.reset();
  
  // Verify node was added
  EXPECT_TRUE(graph->hasNode("graphLockTest"));
}

// Test basic dependencies with edges
TEST_F(CoordinatedGraphMinimalTest, BasicDependencies) {
  // Add nodes
  EXPECT_TRUE(graph->addNode("A", TestData("Node A", 1)));
  EXPECT_TRUE(graph->addNode("B", TestData("Node B", 2)));
  
  // Add edge: A -> B
  EXPECT_TRUE(graph->addEdge("A", "B"));
  
  // Check edge exists
  EXPECT_TRUE(graph->hasEdge("A", "B"));
  
  // Check edge collections
  auto outEdges = graph->getOutEdges("A");
  EXPECT_EQ(outEdges.size(), 1);
  EXPECT_TRUE(outEdges.find("B") != outEdges.end());
  
  auto inEdges = graph->getInEdges("B");
  EXPECT_EQ(inEdges.size(), 1);
  EXPECT_TRUE(inEdges.find("A") != inEdges.end());
}

// Test minimal topological sort
TEST_F(CoordinatedGraphMinimalTest, MinimalTopologicalSort) {
  // Create a simple DAG: A -> B -> C
  EXPECT_TRUE(graph->addNode("A", TestData("Node A", 1)));
  EXPECT_TRUE(graph->addNode("B", TestData("Node B", 2)));
  EXPECT_TRUE(graph->addNode("C", TestData("Node C", 3)));
  
  EXPECT_TRUE(graph->addEdge("A", "B"));
  EXPECT_TRUE(graph->addEdge("B", "C"));
  
  // Get topological ordering
  auto sorted = graph->topologicalSort();
  ASSERT_EQ(sorted.size(), 3);
  
  // Check ordering constraints
  EXPECT_EQ(sorted[0], "A");
  EXPECT_EQ(sorted[1], "B");
  EXPECT_EQ(sorted[2], "C");
}

} // namespace Test
} // namespace Fabric