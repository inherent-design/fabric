#pragma once

#include "fabric/core/Resource.hh" 
#include "fabric/utils/CoordinatedGraph.hh"
#include <any>
#include <atomic>
#include <chrono>
#include <functional>
#include <future>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <type_traits>
#include <unordered_map>
#include <vector>

namespace Fabric {

/**
 * @brief Central hub for managing resources with dependency tracking
 *
 * ResourceHub manages loading, unloading, and tracking dependencies between
 * resources using a thread-safe graph structure. It provides both synchronous
 * and asynchronous resource loading options.
 */
class ResourceHub {
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
  ResourceHandle<T> load(const std::string &typeId,
                         const std::string &resourceId) {
    static_assert(std::is_base_of<Resource, T>::value,
                  "T must be derived from Resource");

    std::shared_ptr<Resource> resource;

    // First, check if the resource exists in the graph
    auto resourceNode = resourceGraph_.getNode(resourceId);

    if (!resourceNode) {
      // Resource doesn't exist, create it
      resource = ResourceFactory::create(typeId, resourceId);
      if (!resource) {
        // Failed to create resource
        return ResourceHandle<T>();
      }

      // Add to graph
      bool added = resourceGraph_.addNode(resourceId, resource);
      if (!added) {
        // Node may have been added by another thread, try to get it again
        resourceNode = resourceGraph_.getNode(resourceId);
        if (resourceNode) {
          // Get the resource from the node
          auto nodeLock = resourceNode->tryLock(
              CoordinatedGraph<std::shared_ptr<Resource>>::LockIntent::Read);
          if (nodeLock && nodeLock->isLocked()) {
            resource = nodeLock->getNode()->getData();
          } else {
            // Could not lock the node, return empty handle
            return ResourceHandle<T>();
          }
        } else {
          // Something went wrong, return empty handle
          return ResourceHandle<T>();
        }
      } else {
        // Get the node we just added
        resourceNode = resourceGraph_.getNode(resourceId);
      }
    } else {
      // Get the resource from the node under a lock
      auto nodeLock = resourceNode->tryLock(
          CoordinatedGraph<std::shared_ptr<Resource>>::LockIntent::Read);
      if (nodeLock && nodeLock->isLocked()) {
        resource = nodeLock->getNode()->getData();
      } else {
        // Could not lock the node, return empty handle
        return ResourceHandle<T>();
      }
    }

    if (!resource) {
      // Something went wrong
      return ResourceHandle<T>();
    }

    // Load the resource if needed
    if (resource->getState() != ResourceState::Loaded) {
      try {
        // Call the load method to actually load the resource
        bool loadSuccess = resource->load();

        if (!loadSuccess) {
          // Log or handle load failure
          std::cerr << "Failed to load resource: " << resourceId << std::endl;
        }

        // Touch the node to update its access time
        if (resourceNode) {
          resourceNode->touch();
        }

        // Check if we need to enforce the memory budget
        try {
          enforceBudget();
        } catch (const std::exception &e) {
          // Log but continue if budget enforcement fails
          std::cerr << "Error enforcing memory budget: " << e.what()
                    << std::endl;
        }
      } catch (const std::exception &e) {
        // Log loading error but continue with the resource handle
        std::cerr << "Error loading resource " << resourceId << ": " << e.what()
                  << std::endl;
      }
    }

    return ResourceHandle<T>(std::static_pointer_cast<T>(resource),
                             &ResourceHub::instance());
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
    static_assert(std::is_base_of<Resource, T>::value,
                  "T must be derived from Resource");

    // First check if the resource is already loaded
    auto resourceNode = resourceGraph_.getNode(resourceId);
    if (resourceNode) {
      auto nodeLock = resourceNode->tryLock(
          CoordinatedGraph<std::shared_ptr<Resource>>::LockIntent::Read);
      if (nodeLock && nodeLock->isLocked()) {
        auto resource = nodeLock->getNode()->getData();
        if (resource->getState() == ResourceState::Loaded) {
          if (callback) {
            callback(ResourceHandle<T>(std::static_pointer_cast<T>(resource),
                                       &ResourceHub::instance()));
          }
          return;
        }
      }
    }

    // Create a load request
    ResourceLoadRequest request;
    request.typeId = typeId;
    request.resourceId = resourceId;
    request.priority = priority;

    if (callback) {
      request.callback = [callback](std::shared_ptr<Resource> resource) {
        callback(ResourceHandle<T>(std::static_pointer_cast<T>(resource),
                                   &ResourceHub::instance()));
      };
    }

    // Add the request to the queue
    {
      std::lock_guard<std::mutex> lock(queueMutex_);
      loadQueue_.push(request);
    }

    // Signal the worker thread
    queueCondition_.notify_one();
  }

  /**
   * @brief Add a dependency between two resources
   *
   * @param dependentId ID of the dependent resource
   * @param dependencyId ID of the dependency
   * @return true if dependency was added, false if either resource doesn't
   * exist or dependecy already exists
   */
  bool addDependency(const std::string &dependentId,
                     const std::string &dependencyId);

  /**
   * @brief Remove a dependency between two resources
   *
   * @param dependentId ID of the dependent resource
   * @param dependencyId ID of the dependency
   * @return true if dependency was removed, false if either resource doesn't
   * exist or there was no dependency
   */
  bool removeDependency(const std::string &dependentId,
                        const std::string &dependencyId);

  /**
   * @brief Unload a resource
   *
   * @param resourceId Resource identifier
   * @return true if the resource was unloaded
   */
  bool unload(const std::string &resourceId);

  /**
   * @brief Unload a resource with an option to cascade unload dependencies
   *
   * @param resourceId Resource identifier
   * @param cascade If true, also unload resources that depend on this one
   * @return true if the resource was unloaded
   */
  bool unload(const std::string &resourceId, bool cascade);

  /**
   * @brief Unload a resource and all resources that depend on it
   *
   * @param resourceId Resource identifier
   * @return true if the resource was unloaded
   */
  bool unloadRecursive(const std::string &resourceId);

  /**
   * @brief Preload a batch of resources asynchronously
   *
   * @param typeIds Type identifiers for each resource
   * @param resourceIds Resource identifiers
   * @param priority Loading priority
   */
  void preload(const std::vector<std::string> &typeIds,
               const std::vector<std::string> &resourceIds,
               ResourcePriority priority = ResourcePriority::Low);

  /**
   * @brief Set the memory budget for the resource manager
   *
   * @param bytes Memory budget in bytes
   */
  void setMemoryBudget(size_t bytes);

  /**
   * @brief Get the memory budget
   *
   * @return Memory budget in bytes
   */
  size_t getMemoryBudget() const;

  /**
   * @brief Get the current memory usage
   *
   * @return Memory usage in bytes
   */
  size_t getMemoryUsage() const;

  /**
   * @brief Explicitly trigger memory budget enforcement
   *
   * @return The number of resources evicted
   */
  size_t enforceMemoryBudget();

  /**
   * @brief Disable worker threads for testing
   */
  void disableWorkerThreadsForTesting();

  /**
   * @brief Restart worker threads after testing
   */
  void restartWorkerThreadsAfterTesting();

  /**
   * @brief Get the number of worker threads
   *
   * @return Number of worker threads
   */
  unsigned int getWorkerThreadCount() const;

  /**
   * @brief Set the number of worker threads
   *
   * @param count Number of worker threads
   */
  void setWorkerThreadCount(unsigned int count);

  /**
   * @brief Get resources that depend on a specific resource
   *
   * @param resourceId Resource identifier
   * @return Set of resource IDs that depend on the specified resource
   */
  std::unordered_set<std::string> getDependents(const std::string &resourceId);

  /**
   * @brief Get resources that a specific resource depends on
   *
   * @param resourceId Resource identifier
   * @return Set of resource IDs that the specified resource depends on
   */
  std::unordered_set<std::string>
  getDependencies(const std::string &resourceId);

  /**
   * @brief Check if a resource exists
   *
   * @param resourceId Resource identifier
   * @return true if the resource exists
   */
  bool hasResource(const std::string &resourceId);

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
  std::vector<std::string>
  getDependentResources(const std::string &resourceId) const;

  /**
   * @brief Get dependency resources as a vector
   *
   * @param resourceId Resource identifier
   * @return Vector of resource IDs that the specified resource depends on
   */
  std::vector<std::string>
  getDependencyResources(const std::string &resourceId) const;

  /**
   * @brief Clear all resources
   *
   * This method unloads and removes all resources from the manager.
   */
  void clear();

  /**
   * @brief Shutdown the resource manager
   *
   * This method stops all worker threads and unloads all resources.
   * The ResourceHub will no longer be usable after this call.
   */
  void shutdown();

private:
  ResourceHub();

  ~ResourceHub();

  // Process load queue function
  void processLoadQueue();

  // Worker thread function
  void workerThreadFunc();

  // Enforce budget
  void enforceBudget();

  // Static mutex
  static std::mutex mutex_;

  // Resource graph
  CoordinatedGraph<std::shared_ptr<Resource>> resourceGraph_;

  // Memory management
  std::atomic<size_t> memoryBudget_;

  // Worker threads
  std::atomic<unsigned int> workerThreadCount_;
  std::vector<std::unique_ptr<std::thread>> workerThreads_;

  // Load queue
  std::priority_queue<ResourceLoadRequest, std::vector<ResourceLoadRequest>,
                      ResourceLoadRequestComparator>
      loadQueue_;

  // Synchronization
  std::mutex queueMutex_;
  std::mutex threadControlMutex_; // Mutex for thread creation/destruction
  std::condition_variable queueCondition_;
  std::atomic<bool> shutdown_{false};
};

/**
 * @brief Create a resource handle with convenience functions using ResourceHub
 *
 * @tparam T Resource type
 * @param typeId Type identifier
 * @param resourceId Resource identifier
 * @return ResourceHandle for the resource
 */
template <typename T>
ResourceHandle<T> loadResource(const std::string &typeId,
                               const std::string &resourceId) {
  return ResourceHub::instance().load<T>(typeId, resourceId);
}

/**
 * @brief Load a resource asynchronously with convenience functions using
 * ResourceHub
 *
 * @tparam T Resource type
 * @param typeId Type identifier
 * @param resourceId Resource identifier
 * @param callback Function to call when the resource is loaded
 * @param priority Loading priority
 */
template <typename T>
void loadResourceAsync(const std::string &typeId, const std::string &resourceId,
                       std::function<void(ResourceHandle<T>)> callback,
                       ResourcePriority priority) {
  ResourceHub::instance().loadAsync<T>(typeId, resourceId, priority, callback);
}

} // namespace Fabric
