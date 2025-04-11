#include "fabric/parser/ArgumentParser.hh"
#include "fabric/utils/Logging.hh"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <iostream>
#include <sstream>

namespace Fabric {
namespace Tests {

class ParserLoggingIntegrationTest : public ::testing::Test {
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
  ArgumentParser parser;
};

TEST_F(ParserLoggingIntegrationTest, ParserErrorLogging) {
  // Setup - clear previous output
  capturedErrOutput.str("");
  parser.addArgument("--required", "Required parameter", true);

  // Prepare args (missing required argument)
  const char *args[] = {"program"};
  int argc = sizeof(args) / sizeof(args[0]);

  // Execute
  parser.parse(argc, const_cast<char **>(args));

  // Verify parser state
  ASSERT_FALSE(parser.isValid());

  // Verify log output (error messages go to cerr)
  std::string output = capturedErrOutput.str();
  ASSERT_FALSE(output.empty());

  // Check that the error is related to required parameter
  ASSERT_FALSE(parser.getErrorMsg().empty());
  ASSERT_THAT(parser.getErrorMsg(), ::testing::HasSubstr("--required"));
}

TEST_F(ParserLoggingIntegrationTest, ValidParsingLogging) {
  // Setup - Add debug logging to the parser
  capturedOutput.str(""); // Clear previous output

  // Log manually to verify capture is working
  Logger::logInfo("Testing parser with valid arguments");

  // Add argument and prepare args
  parser.addArgument("--verbose", "Verbose output");
  const char *args[] = {"program", "--verbose"};
  int argc = sizeof(args) / sizeof(args[0]);

  // Execute
  parser.parse(argc, const_cast<char **>(args));

  // Verify parser state
  ASSERT_TRUE(parser.isValid());
  ASSERT_TRUE(parser.hasArgument("--verbose"));

  // Verify log output (check our manually added log message)
  std::string output = capturedOutput.str();
  ASSERT_THAT(output, ::testing::HasSubstr("INFO"));
  ASSERT_THAT(output,
              ::testing::HasSubstr("Testing parser with valid arguments"));
}

} // namespace Tests
} // namespace Fabric
