#include "fabric/utils/Logging.hh"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <iostream>
#include <sstream>

namespace Fabric {
namespace Tests {

class LoggingTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Initialize logger for tests
    Logger::initialize();

    // Capture cout and cerr for verification
    originalCoutBuf = std::cout.rdbuf();
    originalCerrBuf = std::cerr.rdbuf();
    std::cout.rdbuf(capturedOutput.rdbuf());
    std::cerr.rdbuf(capturedErrOutput.rdbuf());
  }

  void TearDown() override {
    // Restore cout and cerr
    std::cout.rdbuf(originalCoutBuf);
    std::cerr.rdbuf(originalCerrBuf);
  }

  std::stringstream capturedOutput;
  std::stringstream capturedErrOutput;
  std::streambuf *originalCoutBuf;
  std::streambuf *originalCerrBuf;
};

TEST_F(LoggingTest, LogInfo) {
  // Execute
  Logger::logInfo("Info message");

  // Verify
  std::string output = capturedOutput.str();
  ASSERT_THAT(output, ::testing::HasSubstr("INFO"));
  ASSERT_THAT(output, ::testing::HasSubstr("Info message"));
}

TEST_F(LoggingTest, LogWarning) {
  // Execute
  Logger::logWarning("Warning message");

  // Verify
  std::string output = capturedOutput.str();
  ASSERT_THAT(output, ::testing::HasSubstr("WARNING"));
  ASSERT_THAT(output, ::testing::HasSubstr("Warning message"));
}

TEST_F(LoggingTest, LogError) {
  // Clear previous output
  capturedErrOutput.str("");

  // Execute
  Logger::logError("Error message");

  // Verify - Error messages go to std::cerr
  std::string output = capturedErrOutput.str();

  // Check for the error message in cerr output
  ASSERT_THAT(output, ::testing::HasSubstr("ERROR"));
  ASSERT_THAT(output, ::testing::HasSubstr("Error message"));
}

TEST_F(LoggingTest, LogDebug) {
  // Clear previous output
  capturedOutput.str("");

  // Temporarily set the log level to Debug
  LogLevel originalLevel = Logger::getLogLevel();
  Logger::setLogLevel(LogLevel::Debug);

  // Execute
  Logger::logDebug("Debug message");

  // Verify debug message
  std::string output = capturedOutput.str();

  // Restore log level
  Logger::setLogLevel(originalLevel);

#ifndef NDEBUG
  ASSERT_THAT(output, ::testing::HasSubstr("DEBUG"));
  ASSERT_THAT(output, ::testing::HasSubstr("Debug message"));
#else
  // In release builds, just verify we don't crash
  ASSERT_TRUE(true);
#endif
}

} // namespace Tests
} // namespace Fabric
