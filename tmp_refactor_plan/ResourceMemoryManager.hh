#pragma once

#include "fabric/core/Resource.hh"
#include <memory>
#include <string>
#include <vector>

namespace Fabric {

// Forward declaration
class ResourceDependencyManager;

/**
 * @brief Manages memory budgets and resource eviction
 * 
 * This class tracks resource memory usage and handles eviction
 * when memory limits are exceeded.
 */
class ResourceMemoryManager {
public:
    /**
     * @brief Constructor
     * 
     * @param dependencyManager The dependency manager to coordinate with
     */
    explicit ResourceMemoryManager(std::shared_ptr<ResourceDependencyManager> dependencyManager);

    /**
     * @brief Set the memory budget
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
     * This method immediately checks if memory usage exceeds the budget
     * and evicts resources if necessary.
     * 
     * @return The number of resources evicted
     */
    size_t enforceMemoryBudget();
    
    /**
     * @brief Register a resource for memory tracking
     * 
     * @param resourceId Resource identifier
     * @return true if registration succeeded
     */
    bool registerResource(const std::string &resourceId);
    
    /**
     * @brief Unregister a resource from memory tracking
     * 
     * @param resourceId Resource identifier
     * @return true if unregistration succeeded
     */
    bool unregisterResource(const std::string &resourceId);

private:
    // Select resources for eviction based on memory requirements
    std::vector<std::string> selectResourcesForEviction(size_t memoryToFree);
    
    // Memory budget in bytes
    std::atomic<size_t> memoryBudget_;
    
    // Reference to the dependency manager to check for dependencies
    std::shared_ptr<ResourceDependencyManager> dependencyManager_;
    
    // Synchronization mutex for budget enforcement
    mutable std::timed_mutex enforceBudgetMutex_;
};

} // namespace Fabric