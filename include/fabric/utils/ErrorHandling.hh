#pragma once

#include <exception>
#include <string>

namespace Fabric {

/**
 * @brief Custom exception class for Fabric Engine errors
 */
class FabricException : public std::exception {
public:
  /**
   * @brief Construct a new FabricException with a message
   *
   * @param message Error message
   */
  explicit FabricException(const std::string &message);

  /**
   * @brief Get the error message
   *
   * @return Error message
   */
  const char *what() const noexcept override;

private:
  std::string message;
};

/**
 * @brief Throw a FabricException with the given message
 *
 * @param message Error message
 */
[[noreturn]] void throwError(const std::string &message);

} // namespace Fabric
