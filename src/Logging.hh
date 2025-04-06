#pragma once

#include <string>

namespace Fabric
{
  class Logger
  {
  public:
    static void logInfo(const std::string &message);
    static void logWarning(const std::string &message);
    static void logError(const std::string &message);
  };
}