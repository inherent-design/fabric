#pragma once

#include <stdexcept>
#include <string>

class ErrorHandler {
public:
  static void throwError(const std::string &message);
};
