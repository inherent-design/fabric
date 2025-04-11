#include "fabric/utils/ErrorHandling.hh"
#include "gtest/gtest.h"
#include <stdexcept>
#include <string>

// Test fixture for ErrorHandling tests
class ErrorHandlingTest : public ::testing::Test {};

// Test FabricException construction
TEST_F(ErrorHandlingTest, TestFabricExceptionConstruction) {
  // Create a FabricException with a test message
  Fabric::FabricException exception("Test error message");

  // Verify the exception message
  ASSERT_STREQ("Test error message", exception.what());
}

// Test throwError function
TEST_F(ErrorHandlingTest, TestThrowError) {
  // Call throwError and expect a FabricException
  try {
    Fabric::throwError("Test error message");
    FAIL() << "Expected FabricException";
  } catch (const Fabric::FabricException &e) {
    // Verify the exception message
    ASSERT_STREQ("Test error message", e.what());
  } catch (...) {
    FAIL() << "Expected FabricException, got a different exception";
  }
}
