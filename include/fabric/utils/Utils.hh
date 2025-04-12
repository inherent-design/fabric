#pragma once

#include <string>
#include <vector>
#include <mutex>
#include <random>

namespace Fabric {

/**
 * @brief Utility functions for the Fabric Engine
 */
class Utils {
public:
  /**
   * @brief Split a string by delimiter
   * @param str String to split
   * @param delimiter Character to split by
   * @return Vector of string parts
   */
  static std::vector<std::string> splitString(const std::string &str,
                                              char delimiter);

  /**
   * @brief Check if a string starts with a prefix
   * @param str String to check
   * @param prefix Prefix to check for
   * @return True if string starts with prefix
   */
  static bool startsWith(const std::string &str, const std::string &prefix);

  /**
   * @brief Check if a string ends with a suffix
   * @param str String to check
   * @param suffix Suffix to check for
   * @return True if string ends with suffix
   */
  static bool endsWith(const std::string &str, const std::string &suffix);

  /**
   * @brief Trim whitespace from the beginning and end of a string
   * @param str String to trim
   * @return Trimmed string
   */
  static std::string trim(const std::string &str);
  
  /**
   * @brief Generate a unique ID with a prefix
   * 
   * Thread-safe implementation for generating unique identifiers.
   * 
   * @param prefix Prefix to use for the ID
   * @param length Length of the random part of the ID
   * @return Unique ID string
   */
  static std::string generateUniqueId(const std::string& prefix, int length = 8);
};

} // namespace Fabric
