#include "ErrorHandling.h"
#include <iostream>

void ErrorHandler::throwError(const std::string &message) {
  throw std::runtime_error(message);
}

void ErrorHandler::handleError(const std::exception &e) {
  std::cerr << "Error: " << e.what() << std::endl;
}
