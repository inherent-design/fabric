#include "fabric/utils/Utils.hh"
#include "gtest/gtest.h"
#include <string>
#include <vector>

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
