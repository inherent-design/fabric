#include "fabric/core/Constants.g.hh"
#include <array>
#include <cstdlib>
#include <fstream>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <iostream>
#include <string>

namespace Fabric {
namespace Tests {

// Helper function to execute a command and get output
std::string executeCommand(const std::string &command) {
  std::array<char, 128> buffer;
  std::string result;

#ifdef _WIN32
  FILE *pipe = _popen(command.c_str(), "r");
#else
  FILE *pipe = popen(command.c_str(), "r");
#endif

  if (!pipe) {
    return "ERROR: Command execution failed";
  }

  while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
    result += buffer.data();
  }

#ifdef _WIN32
  _pclose(pipe);
#else
  pclose(pipe);
#endif

  return result;
}

class FabricE2ETest : public ::testing::Test {
protected:
  // Path to the Fabric executable (will be built before tests run)
  std::string fabricPath;

  void SetUp() override {
    // Determine the path to the Fabric executable
    // This assumes tests run from the build directory
#ifdef _WIN32
    fabricPath = "..\\bin\\Fabric.exe";
#else
    fabricPath = "../bin/Fabric";
#endif

    // Verify that the executable exists
    std::ifstream fabricFile(fabricPath);
    if (!fabricFile.good()) {
      FAIL() << "Fabric executable not found at: " << fabricPath;
    }
  }
};

// Test help flag
TEST_F(FabricE2ETest, DISABLED_HelpFlag) {
  // Execute Fabric with help flag
  std::string output = executeCommand(fabricPath + " --help");

  // Verify output
  ASSERT_THAT(output,
              ::testing::HasSubstr("Usage: " +
                                   std::string(Fabric::APP_EXECUTABLE_NAME)));
  ASSERT_THAT(output, ::testing::HasSubstr("--version"));
  ASSERT_THAT(output, ::testing::HasSubstr("--help"));
}

// Test version flag
TEST_F(FabricE2ETest, DISABLED_VersionFlag) {
  // Execute Fabric with version flag
  std::string output = executeCommand(fabricPath + " --version");

  // Verify output
  ASSERT_THAT(output, ::testing::HasSubstr(Fabric::APP_NAME));
  ASSERT_THAT(output, ::testing::HasSubstr(Fabric::APP_VERSION));
}

// Note: These tests are marked as DISABLED_ because they require the actual
// executable to be built first. They can be enabled manually for actual E2E
// testing.

} // namespace Tests
} // namespace Fabric
