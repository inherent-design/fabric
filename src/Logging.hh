#pragma once

#include <string>
#include <vector>

namespace Fabric {
/**
 * @brief Log levels for the logging system
 */
enum class LogLevel {
  Debug,   // Detailed information for debugging
  Info,    // General information about system operation
  Warning, // Potential issues that don't prevent operation
  Error,   // Errors that may prevent some functionality
  Critical // Critical errors that prevent operation
};

/**
 * @brief Logging system for the Fabric Engine
 *
 * This class provides static methods for logging messages at different
 * severity levels. It supports configurable log levels and formatting.
 */
class Logger {
public:
  /**
   * @brief Set the minimum log level to display
   *
   * @param level Minimum log level
   */
  static void setLogLevel(LogLevel level);

  /**
   * @brief Get the current minimum log level
   *
   * @return Current log level
   */
  static LogLevel getLogLevel();

  /**
   * @brief Enable or disable timestamps in log messages
   *
   * @param enable Whether to include timestamps
   */
  static void enableTimestamps(bool enable);

  /**
   * @brief Log a debug message
   *
   * @param message Message to log
   */
  static void logDebug(const std::string &message);

  /**
   * @brief Log an informational message
   *
   * @param message Message to log
   */
  static void logInfo(const std::string &message);

  /**
   * @brief Log a warning message
   *
   * @param message Message to log
   */
  static void logWarning(const std::string &message);

  /**
   * @brief Log an error message
   *
   * @param message Message to log
   */
  static void logError(const std::string &message);

  /**
   * @brief Log a critical error message
   *
   * @param message Message to log
   */
  static void logCritical(const std::string &message);

  /**
   * @brief Log a message with a specific log level
   *
   * @param level Log level
   * @param message Message to log
   */
  static void log(LogLevel level, const std::string &message);

private:
  static LogLevel currentLogLevel;
  static bool includeTimestamps;

  /**
   * @brief Format a log message with level prefix and optional timestamp
   *
   * @param level Log level
   * @param message Message to format
   * @return Formatted message
   */
  static std::string formatLogMessage(LogLevel level,
                                      const std::string &message);

  /**
   * @brief Get the current timestamp as a string
   *
   * @return Current timestamp
   */
  static std::string getTimestamp();

  /**
   * @brief Convert a log level to a string
   *
   * @param level Log level
   * @return String representation of the log level
   */
  static std::string logLevelToString(LogLevel level);
};
} // namespace Fabric
