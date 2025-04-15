#pragma once

#include "fabric/core/Resource.hh"
#include "fabric/core/ResourceHub.hh"

namespace Fabric {

/**
 * @brief Create a resource handle with convenience functions
 * 
 * @tparam T Resource type
 * @param typeId Type identifier
 * @param resourceId Resource identifier
 * @return ResourceHandle for the resource
 */
template <typename T>
ResourceHandle<T> loadResource(const std::string& typeId, const std::string& resourceId) {
  return ResourceHub::instance().load<T>(typeId, resourceId);
}

/**
 * @brief Load a resource asynchronously with convenience functions
 * 
 * @tparam T Resource type
 * @param typeId Type identifier
 * @param resourceId Resource identifier
 * @param callback Function to call when the resource is loaded
 * @param priority Loading priority
 */
template <typename T>
void loadResourceAsync(
  const std::string& typeId,
  const std::string& resourceId,
  std::function<void(ResourceHandle<T>)> callback,
  ResourcePriority priority
) {
  ResourceHub::instance().loadAsync<T>(typeId, resourceId, priority, callback);
}

} // namespace Fabric