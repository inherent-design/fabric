#ifndef LOGGING_H
#define LOGGING_H

#include <string>

class Logger {
public:
  static void logInfo(const std::string &message);
  static void logWarning(const std::string &message);
  static void logError(const std::string &message);
};

#endif // LOGGING_H
