#ifndef ERROR_HANDLING_H
#define ERROR_HANDLING_H

#include <stdexcept>
#include <string>

class ErrorHandler {
public:
  static void throwError(const std::string &message);
  static void handleError(const std::exception &e);
};

#endif // ERROR_HANDLING_H
