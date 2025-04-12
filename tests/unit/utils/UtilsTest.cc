#include "fabric/utils/Utils.hh"
#include "gtest/gtest.h"
#include <string>
#include <vector>
#include <unordered_set>
#include <thread>
#include <future>

// Test fixture for Utils tests
class UtilsTest : public ::testing::Test {};

// Test splitting a string
TEST_F(UtilsTest, TestSplitString) {
  // Create a test string
  std::string test = "one,two,three,four";

  // Split the string
  std::vector<std::string> result = Fabric::Utils::splitString(test, ',');

  // Verify the result
  ASSERT_EQ(4, result.size());
  ASSERT_EQ("one", result[0]);
  ASSERT_EQ("two", result[1]);
  ASSERT_EQ("three", result[2]);
  ASSERT_EQ("four", result[3]);
}

// Test empty elements in split string
TEST_F(UtilsTest, TestSplitStringWithEmptyElements) {
  // Create a test string with empty elements
  std::string test = "one,,three,";

  // Split the string
  std::vector<std::string> result = Fabric::Utils::splitString(test, ',');

  // Verify the result - empty elements should be skipped
  ASSERT_EQ(2, result.size());
  ASSERT_EQ("one", result[0]);
  ASSERT_EQ("three", result[1]);
}

// Test empty string
TEST_F(UtilsTest, TestSplitEmptyString) {
  // Create an empty string
  std::string test = "";

  // Split the string
  std::vector<std::string> result = Fabric::Utils::splitString(test, ',');

  // Verify the result - should be an empty vector
  ASSERT_EQ(0, result.size());
}

// Test startsWith
TEST_F(UtilsTest, TestStartsWith) {
  // Test cases
  ASSERT_TRUE(Fabric::Utils::startsWith("Hello, World!", "Hello"));
  ASSERT_TRUE(Fabric::Utils::startsWith("Hello", "Hello"));
  ASSERT_FALSE(Fabric::Utils::startsWith("Hello, World!", "World"));
  ASSERT_FALSE(Fabric::Utils::startsWith("Hello", "HelloWorld"));
  ASSERT_FALSE(Fabric::Utils::startsWith("", "Hello"));
  ASSERT_TRUE(Fabric::Utils::startsWith("Hello", ""));
}

// Test endsWith
TEST_F(UtilsTest, TestEndsWith) {
  // Test cases
  ASSERT_TRUE(Fabric::Utils::endsWith("Hello, World!", "World!"));
  ASSERT_TRUE(Fabric::Utils::endsWith("Hello", "Hello"));
  ASSERT_FALSE(Fabric::Utils::endsWith("Hello, World!", "Hello"));
  ASSERT_FALSE(Fabric::Utils::endsWith("Hello", "WorldHello"));
  ASSERT_FALSE(Fabric::Utils::endsWith("", "Hello"));
  ASSERT_TRUE(Fabric::Utils::endsWith("Hello", ""));
}

// Test trim
TEST_F(UtilsTest, TestTrim) {
  // Test cases
  ASSERT_EQ("Hello, World!", Fabric::Utils::trim("Hello, World!"));
  ASSERT_EQ("Hello, World!", Fabric::Utils::trim("  Hello, World!"));
  ASSERT_EQ("Hello, World!", Fabric::Utils::trim("Hello, World!  "));
  ASSERT_EQ("Hello, World!", Fabric::Utils::trim("  Hello, World!  "));
  ASSERT_EQ("", Fabric::Utils::trim(""));
  ASSERT_EQ("", Fabric::Utils::trim("    "));
  ASSERT_EQ("Hello", Fabric::Utils::trim(" \t\n\r Hello \t\n\r "));
}

// Test generateUniqueId - basic functionality
TEST_F(UtilsTest, TestGenerateUniqueId) {
  // Generate IDs with different prefixes
  std::string id1 = Fabric::Utils::generateUniqueId("test_");
  std::string id2 = Fabric::Utils::generateUniqueId("test_");
  
  // Verify IDs are not empty and have the correct prefix
  ASSERT_FALSE(id1.empty());
  ASSERT_FALSE(id2.empty());
  ASSERT_TRUE(Fabric::Utils::startsWith(id1, "test_"));
  ASSERT_TRUE(Fabric::Utils::startsWith(id2, "test_"));
  
  // Verify IDs are different (uniqueness)
  ASSERT_NE(id1, id2);
  
  // Verify length based on parameter
  std::string id3 = Fabric::Utils::generateUniqueId("prefix_", 4);
  ASSERT_EQ(id3.length(), 11); // "prefix_" (7) + 4 hex digits
}

// Test generateUniqueId - thread safety and uniqueness
TEST_F(UtilsTest, TestGenerateUniqueIdThreadSafety) {
  const int numThreads = 10;
  const int idsPerThread = 100;
  std::unordered_set<std::string> generatedIds;
  std::mutex idsMutex;
  
  auto generateIdsTask = [&]() {
    std::vector<std::string> threadIds;
    threadIds.reserve(idsPerThread);
    
    for (int i = 0; i < idsPerThread; i++) {
      threadIds.push_back(Fabric::Utils::generateUniqueId("thread_"));
    }
    
    // Add to the shared set with a lock
    std::lock_guard<std::mutex> lock(idsMutex);
    for (const auto& id : threadIds) {
      generatedIds.insert(id);
    }
  };
  
  // Launch threads
  std::vector<std::future<void>> futures;
  for (int i = 0; i < numThreads; i++) {
    futures.push_back(std::async(std::launch::async, generateIdsTask));
  }
  
  // Wait for all threads to complete
  for (auto& future : futures) {
    future.wait();
  }
  
  // Verify we have the expected number of unique IDs
  ASSERT_EQ(generatedIds.size(), numThreads * idsPerThread);
}
