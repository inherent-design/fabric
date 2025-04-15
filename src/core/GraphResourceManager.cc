#include "fabric/core/GraphResourceManager.hh"
#include "fabric/utils/Logging.hh"
#include "fabric/utils/ErrorHandling.hh"
#include <algorithm>
#include <iostream>

namespace Fabric {

// Initialize static members if needed
std::mutex GraphResourceManager::mutex_;

// Worker thread function
void GraphResourceManager::workerThreadFunc() {
    // Call the process queue function
    this->processLoadQueue();
}

// Enforce budget wrapper
void GraphResourceManager::enforceBudget() {
    enforceMemoryBudget();
}

// Destructor implementation
GraphResourceManager::~GraphResourceManager() {
    shutdown();
}

// Shutdown implementation
void GraphResourceManager::shutdown() {
    // Signal worker threads to stop
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        shutdown_ = true;
    }
    queueCondition_.notify_all();
    
    // Wait for threads to finish
    for (auto& thread : workerThreads_) {
        if (thread && thread->joinable()) {
            thread->join();
        }
    }
    workerThreads_.clear();
    
    // Unload all resources in proper dependency order
    clear();
}

// Implementation of other non-template methods
bool GraphResourceManager::addDependency(const std::string& dependentId, const std::string& dependencyId) {
    try {
        return resourceGraph_.addEdge(dependentId, dependencyId);
    } catch (const CycleDetectedException& e) {
        // Log the cycle detection
        return false;
    }
}

bool GraphResourceManager::removeDependency(const std::string& dependentId, const std::string& dependencyId) {
    return resourceGraph_.removeEdge(dependentId, dependencyId);
}

bool GraphResourceManager::unload(const std::string& resourceId, bool cascade) {
    if (cascade) {
        // Unload in dependency order
        return unloadRecursive(resourceId);
    } else {
        auto resourceNode = resourceGraph_.getNode(resourceId);
        if (!resourceNode) {
            // Resource not found
            return false;
        }
        
        auto resource = resourceNode->getData();
        
        // Check if there are dependencies on this resource
        auto dependents = resourceGraph_.getInEdges(resourceId);
        if (!dependents.empty()) {
            // Can't unload if other resources depend on this one
            return false;
        }
        
        // Unload the resource
        ResourceState state = resource->getState();
        if (state == ResourceState::Loaded) {
            resource->unload();
        }
        
        // Remove from graph
        return resourceGraph_.removeNode(resourceId);
    }
}

bool GraphResourceManager::unloadRecursive(const std::string& resourceId) {
    // Get dependencies in topological order to ensure proper unloading
    std::vector<std::string> unloadOrder;
    
    // Helper function for DFS traversal
    std::function<void(const std::string&, std::unordered_set<std::string>&)> collectDependents;
    collectDependents = [&](const std::string& id, std::unordered_set<std::string>& visited) {
        visited.insert(id);
        
        // First recurse to all dependents
        auto dependents = resourceGraph_.getInEdges(id);
        for (const auto& dependent : dependents) {
            if (visited.find(dependent) == visited.end()) {
                collectDependents(dependent, visited);
            }
        }
        
        // Then add this resource
        unloadOrder.push_back(id);
    };
    
    // Collect dependents of this resource
    std::unordered_set<std::string> visited;
    collectDependents(resourceId, visited);
    
    // Unload in topological order
    bool success = true;
    for (const auto& id : unloadOrder) {
        auto node = resourceGraph_.getNode(id);
        if (node) {
            auto res = node->getData();
            if (res->getState() == ResourceState::Loaded) {
                res->unload();
            }
            success &= resourceGraph_.removeNode(id);
        }
    }
    
    return success;
}

void GraphResourceManager::preload(
    const std::vector<std::string>& typeIds,
    const std::vector<std::string>& resourceIds,
    ResourcePriority priority
) {
    if (typeIds.size() != resourceIds.size()) {
        throw std::invalid_argument("typeIds and resourceIds must have the same size");
    }
    
    for (size_t i = 0; i < resourceIds.size(); ++i) {
        ResourceLoadRequest request;
        request.typeId = typeIds[i];
        request.resourceId = resourceIds[i];
        request.priority = priority;
        
        // Add the request to the queue
        {
            std::lock_guard<std::mutex> lock(queueMutex_);
            loadQueue_.push(request);
        }
    }
    
    // Signal the worker thread
    queueCondition_.notify_one();
}

void GraphResourceManager::setMemoryBudget(size_t bytes) {
    memoryBudget_ = bytes;
    // When we set a new budget, check if we need to enforce it
    enforceBudget();
}

size_t GraphResourceManager::getMemoryUsage() const {
    size_t total = 0;
    
    // Get all resources from the graph
    auto allResourceIds = resourceGraph_.getAllNodes();
    
    for (const auto& id : allResourceIds) {
        auto node = resourceGraph_.getNode(id);
        if (!node) {
            continue;
        }
        
        auto resource = node->getData();
        if (resource->getState() == ResourceState::Loaded) {
            total += resource->getMemoryUsage();
        }
    }
    
    return total;
}

size_t GraphResourceManager::getMemoryBudget() const {
    return memoryBudget_;
}

bool GraphResourceManager::isLoaded(const std::string& resourceId) const {
    auto node = resourceGraph_.getNode(resourceId);
    if (!node) {
        return false;
    }
    
    auto resource = node->getData();
    return resource->getState() == ResourceState::Loaded;
}

std::vector<std::string> GraphResourceManager::getDependentResources(const std::string& resourceId) const {
    auto dependents = resourceGraph_.getInEdges(resourceId);
    return std::vector<std::string>(dependents.begin(), dependents.end());
}

std::vector<std::string> GraphResourceManager::getDependencyResources(const std::string& resourceId) const {
    auto dependencies = resourceGraph_.getOutEdges(resourceId);
    return std::vector<std::string>(dependencies.begin(), dependencies.end());
}

size_t GraphResourceManager::enforceMemoryBudget() {
    // This mutex ensures only one thread performs eviction at a time
    static std::mutex enforceBudgetMutex;
    std::unique_lock<std::mutex> budgetLock(enforceBudgetMutex, std::try_to_lock);
    
    // If another thread is already enforcing the budget, skip this invocation
    if (!budgetLock.owns_lock()) {
        return 0;
    }
    
    // Check if we need to enforce the budget
    size_t currentUsage = getMemoryUsage();
    if (currentUsage <= memoryBudget_) {
        return 0;
    }
    
    // Calculate how much memory we need to free
    size_t toFree = currentUsage - memoryBudget_;
    
    // Get all resources
    auto allResourceIds = resourceGraph_.getAllNodes();
    
    // Collect eviction candidates sorted by access time
    struct EvictionCandidate {
        std::string id;
        std::chrono::steady_clock::time_point lastAccessTime;
        size_t size;
    };
    
    std::vector<EvictionCandidate> candidates;
    candidates.reserve(allResourceIds.size());
    
    // First phase: collect candidates without modifying anything
    for (const auto& id : allResourceIds) {
        auto node = resourceGraph_.getNode(id);
        if (!node) {
            continue;
        }
        
        // Take a shared lock on the node to read data
        auto nodeLock = node->lockShared();
        auto resource = node->getData();
        
        // Only consider resources that are loaded and have a single reference
        if (resource->getState() == ResourceState::Loaded && resource.use_count() == 1) {
            // Check dependencies outside the node lock to prevent deadlocks
            nodeLock.unlock();
            
            // No dependencies on this resource, safe to evict
            if (resourceGraph_.getInEdges(id).empty()) {
                // Take the lock again to get the lastAccessTime
                nodeLock = node->lockShared();
                candidates.push_back({
                    id,
                    node->getLastAccessTime(),
                    resource->getMemoryUsage()
                });
            }
        }
    }
    
    // Sort by last access time (oldest first)
    std::sort(candidates.begin(), candidates.end(), 
             [](const EvictionCandidate& a, const EvictionCandidate& b) {
                 return a.lastAccessTime < b.lastAccessTime;
             });
    
    // Second phase: evict resources until we've freed enough memory
    size_t evictedCount = 0;
    size_t freedMemory = 0;
    
    for (const auto& candidate : candidates) {
        auto node = resourceGraph_.getNode(candidate.id);
        if (!node) {
            continue;
        }
        
        // Take an exclusive lock on the node before modifying it
        auto nodeLock = node->lockExclusive();
        auto resource = node->getData();
        
        // Double-check conditions under exclusive lock
        if (resource.use_count() > 1 || resource->getState() != ResourceState::Loaded) {
            continue;
        }
        
        // Check dependencies again
        nodeLock.unlock(); // Release lock to avoid deadlocks when checking dependencies
        if (!resourceGraph_.getInEdges(candidate.id).empty()) {
            continue;
        }
        
        // Unload the resource
        if (resource->getState() == ResourceState::Loaded) {
            resource->unload();
        }
        
        // Remove from graph
        resourceGraph_.removeNode(candidate.id);
        
        // Update stats
        freedMemory += candidate.size;
        evictedCount++;
        
        // If we've freed enough memory, we can stop
        if (freedMemory >= toFree) {
            break;
        }
    }
    
    return evictedCount;
}

void GraphResourceManager::processLoadQueue() {
    try {
        while (true) {
            ResourceLoadRequest request;
            bool hasRequest = false;
            
            // Get a request from the queue
            {
                std::unique_lock<std::mutex> lock(queueMutex_);
                
                // Wait for a request or shutdown signal
                // Add a timeout to periodically check the shutdown status even if the condition variable isn't notified
                auto waitResult = queueCondition_.wait_for(lock, 
                    std::chrono::milliseconds(500), 
                    [this] { return !loadQueue_.empty() || shutdown_; });
                
                // Check for shutdown first
                if (shutdown_) {
                    break;
                }
                
                // Skip this iteration if no request is available
                if (!waitResult || loadQueue_.empty()) {
                    continue;
                }
                
                // Get the next request
                request = loadQueue_.top();
                loadQueue_.pop();
                hasRequest = true;
            }
            
            // Skip processing if we didn't get a valid request
            if (!hasRequest) {
                continue;
            }
            
            try {
                // Process the request
                auto resourceNode = resourceGraph_.getNode(request.resourceId);
                std::shared_ptr<Resource> resource;
                
                if (!resourceNode) {
                    // Create a new resource
                    resource = ResourceFactory::create(request.typeId, request.resourceId);
                    if (resource) {
                        if (!resourceGraph_.addNode(request.resourceId, resource)) {
                            // Node may have been added by another thread, try to get it again
                            resourceNode = resourceGraph_.getNode(request.resourceId);
                            if (resourceNode) {
                                resource = resourceNode->getData();
                            }
                        } else {
                            resourceNode = resourceGraph_.getNode(request.resourceId);
                        }
                    }
                } else {
                    // Take a shared lock to read the data
                    auto nodeLock = resourceNode->lockShared();
                    resource = resourceNode->getData();
                }
                
                // Load the resource if needed
                if (resource) {
                    ResourceState state = resource->getState();
                    if (state != ResourceState::Loaded) {
                        // Try to load - handle exceptions that might occur
                        try {
                            resource->load();
                            
                            // Update the last access time
                            if (resourceNode) {
                                resourceNode->touch();
                            }
                        } catch (const std::exception& e) {
                            std::cerr << "Error loading resource " << request.resourceId 
                                << ": " << e.what() << std::endl;
                        }
                    }
                }
                
                // Enforce memory budget - handle any exceptions
                try {
                    enforceBudget();
                } catch (const std::exception& e) {
                    std::cerr << "Error enforcing memory budget: " << e.what() << std::endl;
                }
                
                // Call the callback - handle any exceptions
                if (request.callback && resource) {
                    try {
                        request.callback(resource);
                    } catch (const std::exception& e) {
                        std::cerr << "Error in resource callback for " << request.resourceId 
                            << ": " << e.what() << std::endl;
                    }
                }
            } catch (const std::exception& e) {
                // Catch any exception during request processing to keep the thread alive
                std::cerr << "Error processing request for " << request.resourceId 
                    << ": " << e.what() << std::endl;
            } catch (...) {
                // Catch any unknown exception
                std::cerr << "Unknown error processing request for " << request.resourceId << std::endl;
            }
        }
    } catch (const std::exception& e) {
        // Log the error but don't propagate it - this would terminate the thread
        std::cerr << "Fatal worker thread error: " << e.what() << std::endl;
    } catch (...) {
        // Catch any unknown exception
        std::cerr << "Unknown fatal worker thread error" << std::endl;
    }
}

GraphResourceManager& GraphResourceManager::instance() {
    static GraphResourceManager manager;
    return manager;
}

// Constructor implementation
GraphResourceManager::GraphResourceManager()
    : memoryBudget_(1024 * 1024 * 1024), // 1 GB default
      workerThreadCount_(std::thread::hardware_concurrency()),
      shutdown_(false) {
    // Optional debug message
    std::cout << "GraphResourceManager initialized with " << workerThreadCount_ 
              << " worker threads" << std::endl;
              
    // Start worker threads
    for (unsigned int i = 0; i < workerThreadCount_; ++i) {
        workerThreads_.push_back(std::make_unique<std::thread>(
            &GraphResourceManager::workerThreadFunc, this));
    }
}

} // namespace Fabric