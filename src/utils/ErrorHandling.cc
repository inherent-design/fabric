#include "fabric/utils/ErrorHandling.hh"
#include <iostream>
#include <sstream>

namespace Fabric {

FabricException::FabricException(const std::string &message)
    : message(message) {}

const char *FabricException::what() const noexcept { return message.c_str(); }

void throwError(const std::string &message) { throw FabricException(message); }

} // namespace Fabric
