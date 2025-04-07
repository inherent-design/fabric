#include "Logging.hh"
#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace Fabric {
// Initialize static members
LogLevel Logger::currentLogLevel = LogLevel::Info;
bool Logger::includeTimestamps = true;

void Logger::setLogLevel(LogLevel level) { currentLogLevel = level; }

LogLevel Logger::getLogLevel() { return currentLogLevel; }

void Logger::enableTimestamps(bool enable) { includeTimestamps = enable; }

void Logger::logDebug(const std::string &message) {
  log(LogLevel::Debug, message);
}

void Logger::logInfo(const std::string &message) {
  log(LogLevel::Info, message);
}

void Logger::logWarning(const std::string &message) {
  log(LogLevel::Warning, message);
}

void Logger::logError(const std::string &message) {
  log(LogLevel::Error, message);
}

void Logger::logCritical(const std::string &message) {
  log(LogLevel::Critical, message);
}

void Logger::log(LogLevel level, const std::string &message) {
  // Only log if the level is at or above the current log level
  if (static_cast<int>(level) < static_cast<int>(currentLogLevel)) {
    return;
  }

  std::string formattedMessage = formatLogMessage(level, message);

  // Output to the appropriate stream
  if (level == LogLevel::Error || level == LogLevel::Critical) {
    std::cerr << formattedMessage << std::endl;
  } else {
    std::cout << formattedMessage << std::endl;
  }
}

std::string Logger::formatLogMessage(LogLevel level,
                                     const std::string &message) {
  std::stringstream ss;

  // Add timestamp if enabled
  if (includeTimestamps) {
    ss << getTimestamp() << " ";
  }

  // Add log level
  ss << "[" << logLevelToString(level) << "] " << message;

  return ss.str();
}

std::string Logger::getTimestamp() {
  auto now = std::chrono::system_clock::now();
  auto time = std::chrono::system_clock::to_time_t(now);
  auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                now.time_since_epoch()) %
            1000;

  std::stringstream ss;
  ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
  ss << '.' << std::setfill('0') << std::setw(3) << ms.count();

  return ss.str();
}

std::string Logger::logLevelToString(LogLevel level) {
  switch (level) {
  case LogLevel::Debug:
    return "DEBUG";
  case LogLevel::Info:
    return "INFO";
  case LogLevel::Warning:
    return "WARNING";
  case LogLevel::Error:
    return "ERROR";
  case LogLevel::Critical:
    return "CRITICAL";
  default:
    return "UNKNOWN";
  }
}
} // namespace Fabric
