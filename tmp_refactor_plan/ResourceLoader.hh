#pragma once

#include "fabric/core/Resource.hh"
#include "fabric/utils/Logging.hh"
#include <functional>
#include <future>
#include <memory>
#include <string>
#include <vector>

namespace Fabric {

// Forward declarations
class ResourceThreadPool;
class ResourceDependencyManager;

/**
 * @brief Handles loading resources from different sources
 * 
 * This class is responsible for loading resources both synchronously
 * and asynchronously. It uses a ResourceThreadPool for async operations
 * and coordinates with ResourceDependencyManager.
 */
class ResourceLoader {
public:
    /**
     * @brief Constructor
     * 
     * @param threadPool Thread pool for async loading
     * @param dependencyManager Dependency manager for resource dependencies
     */
    ResourceLoader(
        std::shared_ptr<ResourceThreadPool> threadPool,
        std::shared_ptr<ResourceDependencyManager> dependencyManager);

    /**
     * @brief Load a resource synchronously
     * 
     * @tparam T Resource type
     * @param typeId Type identifier
     * @param resourceId Resource identifier
     * @return ResourceHandle for the loaded resource
     */
    template <typename T>
    ResourceHandle<T> load(const std::string &typeId, const std::string &resourceId);

    /**
     * @brief Load a resource asynchronously
     * 
     * @tparam T Resource type
     * @param typeId Type identifier
     * @param resourceId Resource identifier
     * @param priority Loading priority
     * @param callback Function to call when the resource is loaded
     */
    template <typename T>
    void loadAsync(
        const std::string &typeId,
        const std::string &resourceId,
        ResourcePriority priority,
        std::function<void(ResourceHandle<T>)> callback);

    /**
     * @brief Preload a batch of resources asynchronously
     * 
     * @param typeIds Type identifiers for each resource
     * @param resourceIds Resource identifiers
     * @param priority Loading priority
     */
    void preload(
        const std::vector<std::string> &typeIds,
        const std::vector<std::string> &resourceIds,
        ResourcePriority priority = ResourcePriority::Low);

private:
    // Actual resource loading implementation
    std::shared_ptr<Resource> loadResourceImpl(
        const std::string &typeId,
        const std::string &resourceId,
        bool synchronous);

    // References to collaborating components
    std::shared_ptr<ResourceThreadPool> threadPool_;
    std::shared_ptr<ResourceDependencyManager> dependencyManager_;
};

} // namespace Fabric