#pragma once

#include <stdexcept>
#include <string>

namespace Fabric
{
  class ErrorHandler
  {
  public:
    static void throwError(const std::string &message);
  };
}