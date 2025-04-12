#include "fabric/utils/Utils.hh"
#include <algorithm>
#include <cctype>
#include <sstream>
#include <iomanip>

namespace Fabric {

std::vector<std::string> Utils::splitString(const std::string &str,
                                            char delimiter) {
  std::vector<std::string> tokens;
  std::string token;
  std::istringstream tokenStream(str);

  while (std::getline(tokenStream, token, delimiter)) {
    if (!token.empty()) {
      tokens.push_back(token);
    }
  }

  return tokens;
}

bool Utils::startsWith(const std::string &str, const std::string &prefix) {
  if (str.length() < prefix.length()) {
    return false;
  }
  return str.compare(0, prefix.length(), prefix) == 0;
}

bool Utils::endsWith(const std::string &str, const std::string &suffix) {
  if (str.length() < suffix.length()) {
    return false;
  }
  return str.compare(str.length() - suffix.length(), suffix.length(), suffix) ==
         0;
}

std::string Utils::trim(const std::string &str) {
  auto start = std::find_if_not(
      str.begin(), str.end(), [](unsigned char c) { return std::isspace(c); });

  auto end = std::find_if_not(str.rbegin(), str.rend(), [](unsigned char c) {
               return std::isspace(c);
             }).base();

  return (start < end) ? std::string(start, end) : std::string();
}

std::string Utils::generateUniqueId(const std::string& prefix, int length) {
  static std::mutex idMutex;
  static std::random_device rd;
  static std::mt19937 gen(rd());
  static std::uniform_int_distribution<> dis(0, 15);
  
  std::lock_guard<std::mutex> lock(idMutex);
  
  std::stringstream ss;
  ss << prefix;
  
  for (int i = 0; i < length; i++) {
    ss << std::hex << dis(gen);
  }
  
  return ss.str();
}

} // namespace Fabric
