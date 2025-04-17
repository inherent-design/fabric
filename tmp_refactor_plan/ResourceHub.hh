#pragma once

#include "fabric/core/Resource.hh"
#include "fabric/core/resource/ResourceLoader.hh"
#include "fabric/core/resource/ResourceDependencyManager.hh"
#include "fabric/core/resource/ResourceMemoryManager.hh"
#include "fabric/core/resource/ResourceThreadPool.hh"
#include <memory>
#include <string>
#include <vector>

namespace Fabric {

// Forward declarations
namespace Test {
  class ResourceHubTestHelper;
}

/**
 * @brief Central hub for managing resources with dependency tracking
 *
 * ResourceHub is a facade that coordinates between ResourceLoader, 
 * ResourceDependencyManager, ResourceMemoryManager, and ResourceThreadPool
 * to provide a unified interface for resource management.
 */
class ResourceHub {
  // Allow test helper to access protected members
  friend class Fabric::Test::ResourceHubTestHelper;
  
public:
  /**
   * @brief Get the singleton instance
   *
   * @return Reference to the singleton instance
   */
  static ResourceHub &instance();

  /**
   * @brief Load a resource synchronously
   *
   * @tparam T Resource type
   * @param typeId Type identifier
   * @param resourceId Resource identifier
   * @return ResourceHandle for the loaded resource
   */
  template <typename T>
  ResourceHandle<T> load(const std::string &typeId, const std::string &resourceId) {
    return resourceLoader_->load<T>(typeId, resourceId);
  }

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
  void loadAsync(const std::string &typeId, const std::string &resourceId,
                ResourcePriority priority,
                std::function<void(ResourceHandle<T>)> callback) {
    resourceLoader_->loadAsync<T>(typeId, resourceId, priority, callback);
  }

  /**
   * @brief Add a dependency between two resources
   *
   * @param dependentId ID of the dependent resource
   * @param dependencyId ID of the dependency
   * @return true if dependency was added, false if either resource doesn't exist or dependecy already exists
   */
  bool addDependency(const std::string &dependentId, const std::string &dependencyId) {
    return dependencyManager_->addDependency(dependentId, dependencyId);
  }

  /**
   * @brief Remove a dependency between two resources
   *
   * @param dependentId ID of the dependent resource
   * @param dependencyId ID of the dependency
   * @return true if dependency was removed, false if either resource doesn't exist or there was no dependency
   */
  bool removeDependency(const std::string &dependentId, const std::string &dependencyId) {
    return dependencyManager_->removeDependency(dependentId, dependencyId);
  }

  /**
   * @brief Unload a resource
   *
   * @param resourceId Resource identifier
   * @return true if the resource was unloaded
   */
  bool unload(const std::string &resourceId) {
    return unload(resourceId, false);
  }

  /**
   * @brief Unload a resource with an option to cascade unload dependencies
   *
   * @param resourceId Resource identifier
   * @param cascade If true, also unload resources that depend on this one
   * @return true if the resource was unloaded
   */
  bool unload(const std::string &resourceId, bool cascade) {
    return dependencyManager_->removeResource(resourceId, cascade);
  }

  /**
   * @brief Unload a resource and all resources that depend on it
   *
   * @param resourceId Resource identifier
   * @return true if the resource was unloaded
   */
  bool unloadRecursive(const std::string &resourceId) {
    return unload(resourceId, true);
  }

  /**
   * @brief Preload a batch of resources asynchronously
   *
   * @param typeIds Type identifiers for each resource
   * @param resourceIds Resource identifiers
   * @param priority Loading priority
   */
  void preload(const std::vector<std::string> &typeIds,
              const std::vector<std::string> &resourceIds,
              ResourcePriority priority = ResourcePriority::Low) {
    resourceLoader_->preload(typeIds, resourceIds, priority);
  }

  /**
   * @brief Set the memory budget for the resource manager
   *
   * @param bytes Memory budget in bytes
   */
  void setMemoryBudget(size_t bytes) {
    memoryManager_->setMemoryBudget(bytes);
  }

  /**
   * @brief Get the memory budget
   *
   * @return Memory budget in bytes
   */
  size_t getMemoryBudget() const {
    return memoryManager_->getMemoryBudget();
  }

  /**
   * @brief Get the current memory usage
   *
   * @return Memory usage in bytes
   */
  size_t getMemoryUsage() const {
    return memoryManager_->getMemoryUsage();
  }

  /**
   * @brief Explicitly trigger memory budget enforcement
   *
   * @return The number of resources evicted
   */
  size_t enforceMemoryBudget() {
    return memoryManager_->enforceMemoryBudget();
  }

  /**
   * @brief Disable worker threads for testing
   */
  void disableWorkerThreadsForTesting() {
    threadPool_->disableWorkerThreadsForTesting();
  }

  /**
   * @brief Restart worker threads after testing
   */
  void restartWorkerThreadsAfterTesting() {
    threadPool_->restartWorkerThreadsAfterTesting();
  }

  /**
   * @brief Get the number of worker threads
   *
   * @return Number of worker threads
   */
  unsigned int getWorkerThreadCount() const {
    return threadPool_->getWorkerThreadCount();
  }

  /**
   * @brief Set the number of worker threads
   *
   * @param count Number of worker threads
   */
  void setWorkerThreadCount(unsigned int count) {
    threadPool_->setWorkerThreadCount(count);
  }

  /**
   * @brief Get resources that depend on a specific resource
   *
   * @param resourceId Resource identifier
   * @return Set of resource IDs that depend on the specified resource
   */
  std::unordered_set<std::string> getDependents(const std::string &resourceId) {
    return dependencyManager_->getDependents(resourceId);
  }

  /**
   * @brief Get resources that a specific resource depends on
   *
   * @param resourceId Resource identifier
   * @return Set of resource IDs that the specified resource depends on
   */
  std::unordered_set<std::string> getDependencies(const std::string &resourceId) {
    return dependencyManager_->getDependencies(resourceId);
  }

  /**
   * @brief Check if a resource exists
   *
   * @param resourceId Resource identifier
   * @return true if the resource exists
   */
  bool hasResource(const std::string &resourceId) {
    return dependencyManager_->hasResource(resourceId);
  }

  /**
   * @brief Check if a resource is loaded
   *
   * @param resourceId Resource identifier
   * @return true if the resource is loaded
   */
  bool isLoaded(const std::string &resourceId) const;

  /**
   * @brief Get dependent resources as a vector
   *
   * @param resourceId Resource identifier
   * @return Vector of resource IDs that depend on the specified resource
   */
  std::vector<std::string> getDependentResources(const std::string &resourceId) const;

  /**
   * @brief Get dependency resources as a vector
   *
   * @param resourceId Resource identifier
   * @return Vector of resource IDs that the specified resource depends on
   */
  std::vector<std::string> getDependencyResources(const std::string &resourceId) const;

  /**
   * @brief Clear all resources
   *
   * This method unloads and removes all resources from the manager.
   */
  void clear() {
    dependencyManager_->clear();
  }
  
  /**
   * @brief Reset the resource hub to a clean state
   * 
   * This method is useful for testing. It:
   * 1. Disables worker threads
   * 2. Clears all resources
   * 3. Resets the memory budget to the default value
   */
  void reset();
  
  /**
   * @brief Check if the resource hub is empty
   * 
   * @return true if the hub has no resources
   */
  bool isEmpty() const;

  /**
   * @brief Shutdown the resource manager
   *
   * This method stops all worker threads and unloads all resources.
   * The ResourceHub will no longer be usable after this call.
   */
  void shutdown();

protected:
  // Expose the dependency manager for tests
  std::shared_ptr<ResourceDependencyManager> getDependencyManager() {
    return dependencyManager_;
  }

private:
  ResourceHub();
  ~ResourceHub();

  // Component managers
  std::shared_ptr<ResourceLoader> resourceLoader_;
  std::shared_ptr<ResourceDependencyManager> dependencyManager_;
  std::shared_ptr<ResourceMemoryManager> memoryManager_;
  std::shared_ptr<ResourceThreadPool> threadPool_;
};

// Convenience function wrappers
template <typename T>
ResourceHandle<T> loadResource(const std::string &typeId, const std::string &resourceId) {
  return ResourceHub::instance().load<T>(typeId, resourceId);
}

template <typename T>
void loadResourceAsync(const std::string &typeId, const std::string &resourceId,
                    std::function<void(ResourceHandle<T>)> callback,
                    ResourcePriority priority) {
  ResourceHub::instance().loadAsync<T>(typeId, resourceId, priority, callback);
}

} // namespace Fabric