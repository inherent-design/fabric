#include "ErrorHandling.hh"
#include <iostream>
#include <sstream>

namespace Fabric {
void ErrorHandler::throwError(const std::string &message) {
  throw std::runtime_error(message);
}

void ErrorHandler::assertCondition(bool condition, const std::string &message) {
  if (!condition) {
    throwError(message);
  }
}

std::string ErrorHandler::formatError(const std::string &context,
                                      const std::string &message) {
  std::stringstream ss;
  ss << "[" << context << "] " << message;
  return ss.str();
}
} // namespace Fabric
