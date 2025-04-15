#pragma once

#include "fabric/core/Resource.hh"
#include "fabric/utils/ConcurrentGraph.hh"
#include <string>
#include <memory>
#include <unordered_map>
#include <functional>
#include <type_traits>
#include <mutex>
#include <vector>
#include <future>
#include <chrono>
#include <queue>
#include <thread>
#include <any>
#include <atomic>
#include <iostream>

namespace Fabric {

/**
 * @brief Graph-based implementation of ResourceManager
 * 
 * This implementation uses a ConcurrentGraph to track dependencies between resources,
 * enabling fine-grained locking and better performance in multi-threaded environments.
 * It replaces the original ResourceManager implementation.
 */
class GraphResourceManager {
public:
    /**
     * @brief Get the singleton instance
     * 
     * @return Reference to the singleton instance
     */
    static GraphResourceManager& instance();
    
    /**
     * @brief Load a resource synchronously
     * 
     * @tparam T Resource type
     * @param typeId Type identifier
     * @param resourceId Resource identifier
     * @return ResourceHandle for the loaded resource
     */
    template <typename T>
    ResourceHandle<T> load(const std::string& typeId, const std::string& resourceId) {
        static_assert(std::is_base_of<Resource, T>::value, "T must be derived from Resource");
        
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
                    auto nodeLock = resourceNode->lockShared();
                    resource = resourceNode->getData();
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
            auto nodeLock = resourceNode->lockShared();
            resource = resourceNode->getData();
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
                } catch (const std::exception& e) {
                    // Log but continue if budget enforcement fails
                    std::cerr << "Error enforcing memory budget: " << e.what() << std::endl;
                }
            } catch (const std::exception& e) {
                // Log loading error but continue with the resource handle
                std::cerr << "Error loading resource " << resourceId << ": " << e.what() << std::endl;
            }
        }
        
        return ResourceHandle<T>(std::static_pointer_cast<T>(resource), &ResourceManager::instance());
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
    void loadAsync(
        const std::string& typeId,
        const std::string& resourceId,
        ResourcePriority priority = ResourcePriority::Normal,
        std::function<void(ResourceHandle<T>)> callback = nullptr
    ) {
        static_assert(std::is_base_of<Resource, T>::value, "T must be derived from Resource");
        
        // First check if the resource is already loaded
        auto resourceNode = resourceGraph_.getNode(resourceId);
        if (resourceNode) {
            auto resource = resourceNode->getData();
            if (resource->getState() == ResourceState::Loaded) {
                if (callback) {
                    callback(ResourceHandle<T>(std::static_pointer_cast<T>(resource), &ResourceManager::instance()));
                }
                return;
            }
        }
        
        // Create a load request
        ResourceLoadRequest request;
        request.typeId = typeId;
        request.resourceId = resourceId;
        request.priority = priority;
        
        if (callback) {
            request.callback = [callback](std::shared_ptr<Resource> resource) {
                callback(ResourceHandle<T>(std::static_pointer_cast<T>(resource), &ResourceManager::instance()));
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
     * @return true if dependency was added, false if either resource doesn't exist or dependecy already exists
     */
    bool addDependency(const std::string& dependentId, const std::string& dependencyId);
    
    /**
     * @brief Remove a dependency between two resources
     * 
     * @param dependentId ID of the dependent resource
     * @param dependencyId ID of the dependency
     * @return true if dependency was removed, false if either resource doesn't exist or there was no dependency
     */
    bool removeDependency(const std::string& dependentId, const std::string& dependencyId);
    
    /**
     * @brief Unload a resource
     * 
     * @param resourceId Resource identifier
     * @return true if the resource was unloaded
     */
    bool unload(const std::string& resourceId);
    
    /**
     * @brief Unload a resource with an option to cascade unload dependencies
     * 
     * @param resourceId Resource identifier
     * @param cascade If true, also unload resources that depend on this one
     * @return true if the resource was unloaded
     */
    bool unload(const std::string& resourceId, bool cascade);
    
    /**
     * @brief Unload a resource and all resources that depend on it
     * 
     * @param resourceId Resource identifier
     * @return true if the resource was unloaded
     */
    bool unloadRecursive(const std::string& resourceId);
    
    /**
     * @brief Preload a batch of resources asynchronously
     * 
     * @param typeIds Type identifiers for each resource
     * @param resourceIds Resource identifiers
     * @param priority Loading priority
     */
    void preload(
        const std::vector<std::string>& typeIds,
        const std::vector<std::string>& resourceIds,
        ResourcePriority priority = ResourcePriority::Low
    );
    
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
    void disableWorkerThreadsForTesting() {
        // Use mutex to synchronize thread shutdown
        std::unique_lock<std::mutex> qLock(threadControlMutex_);
        
        // Return early if already shut down
        if (workerThreadCount_ == 0 && workerThreads_.empty()) {
            return;
        }
        
        // Signal threads to exit
        {
            std::lock_guard<std::mutex> qLock(queueMutex_);
            shutdown_ = true;
            // Clear any pending requests to prevent blocked workers
            while (!loadQueue_.empty()) {
                loadQueue_.pop();
            }
        }
        queueCondition_.notify_all();
        
        // Join all worker threads with timeout to prevent hangs
        for (auto& thread : workerThreads_) {
            if (thread && thread->joinable()) {
                // Create a timeout thread that interrupts if joining takes too long
                std::atomic<bool> joinCompleted{false};
                std::thread timeoutThread([&]() {
                    for (int i = 0; i < 50; i++) {  // Wait up to 5 seconds
                        if (joinCompleted) return;
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    }
                    if (!joinCompleted) {
                        // Log timeout warning - thread may be deadlocked
                        std::cerr << "Warning: Thread join timeout in disableWorkerThreadsForTesting" << std::endl;
                    }
                });
                
                // Join the worker thread
                thread->join();
                joinCompleted = true;
                
                // Clean up timeout thread
                if (timeoutThread.joinable()) {
                    timeoutThread.join();
                }
            }
        }
        
        workerThreads_.clear();
        workerThreadCount_ = 0;
    }
    
    /**
     * @brief Restart worker threads after testing
     */
    void restartWorkerThreadsAfterTesting() {
        // Use mutex to synchronize thread creation
        std::unique_lock<std::mutex> qLock(threadControlMutex_);
        
        // Make sure any existing threads are properly shut down
        if (!workerThreads_.empty()) {
            // Signal threads to exit
            {
                std::lock_guard<std::mutex> qLock(queueMutex_);
                shutdown_ = true;
                // Clear any pending requests to prevent blocked workers
                while (!loadQueue_.empty()) {
                    loadQueue_.pop();
                }
            }
            queueCondition_.notify_all();
            
            // Join with timeout to prevent hangs
            for (auto& thread : workerThreads_) {
                if (thread && thread->joinable()) {
                    // Create a timeout thread
                    std::atomic<bool> joinCompleted{false};
                    std::thread timeoutThread([&]() {
                        for (int i = 0; i < 50; i++) {  // Wait up to 5 seconds
                            if (joinCompleted) return;
                            std::this_thread::sleep_for(std::chrono::milliseconds(100));
                        }
                        if (!joinCompleted) {
                            // Log timeout warning
                            std::cerr << "Warning: Thread join timeout in restartWorkerThreadsAfterTesting" << std::endl;
                        }
                    });
                    
                    // Join the worker thread
                    thread->join();
                    joinCompleted = true;
                    
                    // Clean up timeout thread
                    if (timeoutThread.joinable()) {
                        timeoutThread.join();
                    }
                }
            }
            workerThreads_.clear();
        }
        
        // Start fresh
        shutdown_ = false;
        workerThreadCount_ = std::thread::hardware_concurrency();
        
        // Start worker threads
        workerThreads_.reserve(workerThreadCount_);
        for (unsigned int i = 0; i < workerThreadCount_; ++i) {
            workerThreads_.push_back(std::make_unique<std::thread>(
                &GraphResourceManager::workerThreadFunc, this));
        }
    }
    
    /**
     * @brief Get the number of worker threads
     * 
     * @return Number of worker threads
     */
    unsigned int getWorkerThreadCount() const {
        return workerThreadCount_;
    }
    
    /**
     * @brief Set the number of worker threads
     * 
     * @param count Number of worker threads
     */
    void setWorkerThreadCount(unsigned int count) {
        if (count == 0) {
            throw std::invalid_argument("Worker thread count must be at least 1");
        }
        
        // Use thread control mutex to synchronize thread adjustments
        std::unique_lock<std::mutex> controlLock(threadControlMutex_);
        
        // If no change, return early
        if (count == workerThreadCount_) {
            return;
        }
        
        // If decreasing thread count, signal specific threads to stop
        if (count < workerThreadCount_) {
            unsigned int threadsToStop = workerThreadCount_ - count;
            std::vector<std::unique_ptr<std::thread>> threadsToJoin;
            
            // Move threads to be stopped to a separate vector
            for (unsigned int i = 0; i < threadsToStop; ++i) {
                if (i < workerThreads_.size()) {
                    threadsToJoin.push_back(std::move(workerThreads_.back()));
                    workerThreads_.pop_back();
                }
            }
            
            // Signal threads to check their shutdown status
            queueCondition_.notify_all();
            
            // Join the threads we're removing with timeout protection
            for (auto& thread : threadsToJoin) {
                if (thread && thread->joinable()) {
                    // Create a timeout thread
                    std::atomic<bool> joinCompleted{false};
                    std::thread timeoutThread([&]() {
                        for (int i = 0; i < 30; i++) {  // 3 second timeout
                            if (joinCompleted) return;
                            std::this_thread::sleep_for(std::chrono::milliseconds(100));
                        }
                        if (!joinCompleted) {
                            // Log timeout warning
                            std::cerr << "Warning: Thread join timeout in setWorkerThreadCount" << std::endl;
                        }
                    });
                    
                    // Join the worker thread
                    thread->join();
                    joinCompleted = true;
                    
                    // Clean up timeout thread
                    if (timeoutThread.joinable()) {
                        timeoutThread.join();
                    }
                }
            }
        }
        
        // If increasing thread count, create new threads
        if (count > workerThreadCount_) {
            // Ensure we're not in shutdown state
            if (shutdown_) {
                shutdown_ = false; // Reset shutdown flag
            }
            
            // Create new threads
            unsigned int threadsToAdd = count - workerThreadCount_;
            workerThreads_.reserve(workerThreads_.size() + threadsToAdd);
            
            for (unsigned int i = 0; i < threadsToAdd; ++i) {
                try {
                    workerThreads_.push_back(std::make_unique<std::thread>(
                        &GraphResourceManager::workerThreadFunc, this));
                } catch (const std::exception& e) {
                    // Log thread creation error
                    std::cerr << "Error creating worker thread: " << e.what() << std::endl;
                }
            }
        }
        
        // Update the thread count
        workerThreadCount_ = count;
    }
    
    /**
     * @brief Get resources that depend on a specific resource
     * 
     * @param resourceId Resource identifier
     * @return Set of resource IDs that depend on the specified resource
     */
    std::unordered_set<std::string> getDependents(const std::string& resourceId);
    
    /**
     * @brief Get resources that a specific resource depends on
     * 
     * @param resourceId Resource identifier
     * @return Set of resource IDs that the specified resource depends on
     */
    std::unordered_set<std::string> getDependencies(const std::string& resourceId);
    
    /**
     * @brief Check if a resource exists
     * 
     * @param resourceId Resource identifier
     * @return true if the resource exists
     */
    bool hasResource(const std::string& resourceId);
    
    /**
     * @brief Check if a resource is loaded
     * 
     * @param resourceId Resource identifier
     * @return true if the resource is loaded
     */
    bool isLoaded(const std::string& resourceId) const;
    
    /**
     * @brief Get dependent resources as a vector
     * 
     * @param resourceId Resource identifier
     * @return Vector of resource IDs that depend on the specified resource
     */
    std::vector<std::string> getDependentResources(const std::string& resourceId) const;
    
    /**
     * @brief Get dependency resources as a vector
     * 
     * @param resourceId Resource identifier
     * @return Vector of resource IDs that the specified resource depends on
     */
    std::vector<std::string> getDependencyResources(const std::string& resourceId) const;
    
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
     * The ResourceManager will no longer be usable after this call.
     */
    void shutdown();
    
private:
    GraphResourceManager();
    
    ~GraphResourceManager();
    
    // Process load queue function
    void processLoadQueue();
    
    // Worker thread function
    void workerThreadFunc();
    
    // Enforce memory budget
    void enforceBudget();
    
    // Static mutex
    static std::mutex mutex_;

    // Resource graph
    ConcurrentGraph<std::shared_ptr<Resource>> resourceGraph_;
    
    // Memory management
    std::atomic<size_t> memoryBudget_;
    
    // Worker threads
    std::atomic<unsigned int> workerThreadCount_;
    std::vector<std::unique_ptr<std::thread>> workerThreads_;
    
    // Load queue
    std::priority_queue<ResourceLoadRequest,
                      std::vector<ResourceLoadRequest>,
                      ResourceLoadRequestComparator> loadQueue_;
    
    // Synchronization
    std::mutex queueMutex_;
    std::mutex threadControlMutex_;  // Mutex for thread creation/destruction
    std::condition_variable queueCondition_;
    std::atomic<bool> shutdown_{false};
};

/**
 * @brief Create a resource handle with convenience functions using GraphResourceManager
 * 
 * @tparam T Resource type
 * @param typeId Type identifier
 * @param resourceId Resource identifier
 * @return ResourceHandle for the resource
 */
template <typename T>
ResourceHandle<T> loadGraphResource(const std::string& typeId, const std::string& resourceId) {
    return GraphResourceManager::instance().load<T>(typeId, resourceId);
}

/**
 * @brief Load a resource asynchronously with convenience functions using GraphResourceManager
 * 
 * @tparam T Resource type
 * @param typeId Type identifier
 * @param resourceId Resource identifier
 * @param callback Function to call when the resource is loaded
 * @param priority Loading priority
 */
template <typename T>
void loadGraphResourceAsync(
    const std::string& typeId,
    const std::string& resourceId,
    std::function<void(ResourceHandle<T>)> callback,
    ResourcePriority priority = ResourcePriority::Normal
) {
    GraphResourceManager::instance().loadAsync<T>(typeId, resourceId, priority, callback);
}

} // namespace Fabric