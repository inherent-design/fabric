#include "Logging.h"
#include <iostream>

void Logger::logInfo(const std::string &message) {
  std::cout << "Info: " << message << std::endl;
}

void Logger::logWarning(const std::string &message) {
  std::cout << "Warning: " << message << std::endl;
}

void Logger::logError(const std::string &message) {
  std::cerr << "Error: " << message << std::endl;
}
