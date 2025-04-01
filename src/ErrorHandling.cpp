#include "ErrorHandling.h"
#include <iostream>

void ErrorHandler::throwError(const std::string &message) {
  throw std::runtime_error(message);
}
