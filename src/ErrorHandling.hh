#pragma once

#include <stdexcept>
#include <string>

namespace Fabric {
/**
 * @brief Error handling utilities for the Fabric Engine
 *
 * This class provides static methods for handling errors in a consistent way
 * throughout the application.
 */
class ErrorHandler {
public:
  /**
   * @brief Throw a runtime error with the given message
   *
   * @param message Error message
   * @throws std::runtime_error with the given message
   */
  static void throwError(const std::string &message);

  /**
   * @brief Check a condition and throw an error if it's false
   *
   * @param condition Condition to check
   * @param message Error message if condition is false
   * @throws std::runtime_error with the given message if condition is false
   */
  static void assertCondition(bool condition, const std::string &message);

  /**
   * @brief Format an error message with context information
   *
   * @param context Context where the error occurred (e.g., function name)
   * @param message Error message
   * @return Formatted error message
   */
  static std::string formatError(const std::string &context,
                                 const std::string &message);
};
} // namespace Fabric
