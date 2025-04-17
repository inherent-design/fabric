#pragma once

#include "fabric/core/Resource.hh" 
#include "fabric/utils/CoordinatedGraph.hh"
#include "fabric/utils/Logging.hh"
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

// Forward declarations
namespace Test {
  class ResourceHubTestHelper;
}

/**
 * @brief Central hub for managing resources with dependency tracking
 *
 * ResourceHub manages loading, unloading, and tracking dependencies between
 * resources using a thread-safe graph structure. It provides both synchronous
 * and asynchronous resource loading options.
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
  ResourceHandle<T> load(const std::string &typeId,
                         const std::string &resourceId) {
    static_assert(std::is_base_of<Resource, T>::value,
                  "T must be derived from Resource");

    // Robust implementation following best practices from docs/guides/IMPLEMENTATION_PATTERNS.md
    // Using the Copy-Then-Process pattern and avoiding nested locks
    
    const int LOAD_TIMEOUT_MS = 500; // 500ms timeout to prevent UI hangs
    const int PHASE_TIMEOUT_MS = 150; // Each phase gets shorter timeout
    auto startTime = std::chrono::steady_clock::now();
    
    // Timeout checker function
    auto isTimedOut = [&startTime, LOAD_TIMEOUT_MS]() -> bool {
      return std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - startTime)
                .count() > LOAD_TIMEOUT_MS;
    };
    
    try {
      // =================================================================
      // Phase 1: Resource Lookup or Creation - Highly resilient to failures
      // =================================================================
      std::shared_ptr<Resource> resource;
      bool createdNewResource = false;
      
      // First attempt to get existing resource
      if (!isTimedOut()) {
        try {
          // Use a very short operation timeout to avoid blocking
          bool nodeExists = false;
          try {
            nodeExists = resourceGraph_.hasNode(resourceId);
          } catch (...) {
            // Silently continue with creation flow if this fails
          }
          
          if (nodeExists) {
            try {
              auto node = resourceGraph_.getNode(resourceId, PHASE_TIMEOUT_MS / 3);
              if (node) {
                auto nodeLock = node->tryLock(
                    CoordinatedGraph<std::shared_ptr<Resource>>::LockIntent::Read,
                    PHASE_TIMEOUT_MS / 3);
                
                if (nodeLock && nodeLock->isLocked()) {
                  resource = nodeLock->getNode()->getData();
                  nodeLock->release(); // Important: Release lock after getting data
                }
              }
            } catch (...) {
              // Silently continue if this fails
            }
          }
        } catch (...) {
          // Silently continue with creation flow if this fails
        }
      }
      
      // If we still don't have a resource by this point, create a new one
      if (!resource && !isTimedOut()) {
        try {
          // Create new resource
          resource = ResourceFactory::create(typeId, resourceId);
          if (resource) {
            createdNewResource = true;
            
            // Try to add resource to graph with strict timeout
            try {
              bool added = resourceGraph_.addNode(resourceId, resource);
              
              // If adding failed, the node might already exist
              if (!added) {
                // Handle the case where someone else added the node first
                try {
                  auto node = resourceGraph_.getNode(resourceId, PHASE_TIMEOUT_MS / 3);
                  if (node) {
                    auto nodeLock = node->tryLock(
                        CoordinatedGraph<std::shared_ptr<Resource>>::LockIntent::Read,
                        PHASE_TIMEOUT_MS / 3);
                        
                    if (nodeLock && nodeLock->isLocked()) {
                      // Use the existing resource instead
                      resource = nodeLock->getNode()->getData();
                      createdNewResource = false;
                      nodeLock->release();
                    }
                  }
                } catch (...) {
                  // If we can't get the node someone else added, keep our locally created one
                  // We just won't be able to add it to the graph
                }
              }
            } catch (...) {
              // If graph operations fail, we still have the resource locally
              // Just continue with local instance
              Logger::logWarning("Failed to add resource to graph: " + resourceId);
            }
          } else {
            Logger::logError("Failed to create resource: " + resourceId);
          }
        } catch (const std::exception &e) {
          Logger::logError("Exception creating resource: " + std::string(e.what()));
        } catch (...) {
          Logger::logError("Unknown exception creating resource");
        }
      }
      
      // Return early if we have no resource or timed out
      if (!resource) {
        if (isTimedOut()) {
          Logger::logWarning("Timed out in ResourceHub::load during resource lookup for " + resourceId);
        } else {
          Logger::logError("Could not create or retrieve resource: " + resourceId);
        }
        return ResourceHandle<T>();
      }
      
      // =================================================================
      // Phase 2: Resource Loading - With proper timeout handling
      // =================================================================
      if (resource->getState() != ResourceState::Loaded && !isTimedOut()) {
        auto loadTimeoutMs = PHASE_TIMEOUT_MS;
        auto loadStartTime = std::chrono::steady_clock::now();
        
        auto loadTimedOut = [&loadStartTime, loadTimeoutMs]() -> bool {
          return std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - loadStartTime)
                    .count() > loadTimeoutMs;
        };
        
        // Load the resource with a separate timeout
        try {
          // Create a future with timeout for loading
          std::promise<bool> loadPromise;
          auto loadFuture = loadPromise.get_future();
          
          // Create a thread to load the resource
          std::thread loadThread([&resource, &loadPromise]() {
            try {
              bool result = resource->load();
              loadPromise.set_value(result);
            } catch (...) {
              try {
                loadPromise.set_value(false);
              } catch (...) {
                // Promise might already be satisfied
              }
            }
          });
          
          // Detach the thread to avoid blocking
          loadThread.detach();
          
          // Wait for the future with timeout
          bool loadSuccess = false;
          
          if (loadFuture.wait_for(std::chrono::milliseconds(loadTimeoutMs)) == 
              std::future_status::ready) {
            try {
              loadSuccess = loadFuture.get();
            } catch (...) {
              loadSuccess = false;
            }
          } else {
            Logger::logWarning("Resource loading timed out for: " + resourceId);
            loadSuccess = false;
          }
          
          if (!loadSuccess) {
            Logger::logWarning("Failed to load resource: " + resourceId);
            // Continue anyway - we'll return the handle even if loading failed
          }
          
          // Update access time if needed and if we haven't timed out
          if ((createdNewResource || loadSuccess) && !loadTimedOut() && !isTimedOut()) {
            try {
              auto node = resourceGraph_.getNode(resourceId, PHASE_TIMEOUT_MS / 3);
              if (node) {
                node->touch();
              }
            } catch (...) {
              // Failure to update access time is not critical
            }
          }
        } catch (const std::exception &e) {
          Logger::logError("Exception during resource loading: " + std::string(e.what()));
        } catch (...) {
          Logger::logError("Unknown exception during resource loading");
        }
      }
      
      // =================================================================
      // Phase 3: Handle Creation
      // =================================================================
      try {
        // Even if loading failed, return a handle to the resource
        // The client can check the resource state
        if (!isTimedOut()) {
          return ResourceHandle<T>(std::static_pointer_cast<T>(resource),
                                  &ResourceHub::instance());
        } else {
          Logger::logWarning("Timed out before returning resource handle: " + resourceId);
          return ResourceHandle<T>();
        }
      } catch (const std::exception &e) {
        Logger::logError("Exception creating resource handle: " + std::string(e.what()));
        return ResourceHandle<T>();
      }
                              
    } catch (const std::exception &e) {
      Logger::logError("Exception in ResourceHub::load() for " + resourceId + ": " + std::string(e.what()));
      return ResourceHandle<T>();
    } catch (...) {
      Logger::logError("Unknown exception in ResourceHub::load() for " + resourceId);
      return ResourceHandle<T>();
    }
    
    // Fallback in case of any uncaught errors
    return ResourceHandle<T>();
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
      std::lock_guard<std::timed_mutex> lock(queueMutex_);
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
  // For testing access - would normally be private but we need it in tests
  CoordinatedGraph<std::shared_ptr<Resource>> resourceGraph_;

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
  static std::timed_mutex mutex_;

  // Memory management
  std::atomic<size_t> memoryBudget_;

  // Worker threads
  std::atomic<unsigned int> workerThreadCount_;
  std::vector<std::unique_ptr<std::thread>> workerThreads_;

  // Load queue
  std::priority_queue<ResourceLoadRequest, std::vector<ResourceLoadRequest>,
                      ResourceLoadRequestComparator>
      loadQueue_;

  // Synchronization with timed mutex support for safer thread management
  std::timed_mutex queueMutex_;
  std::timed_mutex threadControlMutex_; // Mutex for thread creation/destruction
  std::condition_variable_any queueCondition_;
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
