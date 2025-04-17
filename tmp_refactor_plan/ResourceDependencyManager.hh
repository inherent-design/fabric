#pragma once

#include "fabric/utils/CoordinatedGraph.hh"
#include "fabric/core/Resource.hh"
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

namespace Fabric {

/**
 * @brief Manages resource dependencies using a directed acyclic graph
 * 
 * This class tracks dependencies between resources and ensures that
 * resources are loaded and unloaded in the correct order.
 */
class ResourceDependencyManager {
public:
    ResourceDependencyManager();

    /**
     * @brief Add a dependency between two resources
     * 
     * @param dependentId ID of the dependent resource
     * @param dependencyId ID of the dependency
     * @return true if dependency was added, false if either resource doesn't exist or dependency already exists
     */
    bool addDependency(const std::string &dependentId, const std::string &dependencyId);

    /**
     * @brief Remove a dependency between two resources
     * 
     * @param dependentId ID of the dependent resource
     * @param dependencyId ID of the dependency
     * @return true if dependency was removed, false if either resource doesn't exist or there was no dependency
     */
    bool removeDependency(const std::string &dependentId, const std::string &dependencyId);

    /**
     * @brief Check if a resource exists
     * 
     * @param resourceId Resource identifier
     * @return true if the resource exists
     */
    bool hasResource(const std::string &resourceId);

    /**
     * @brief Add a resource to the dependency graph
     * 
     * @param resourceId Resource identifier
     * @param resource Resource pointer
     * @return true if the resource was added, false if it already exists
     */
    bool addResource(const std::string &resourceId, std::shared_ptr<Resource> resource);

    /**
     * @brief Remove a resource from the dependency graph
     * 
     * @param resourceId Resource identifier
     * @param cascade If true, also remove resources that depend on this one
     * @return true if resource was removed
     */
    bool removeResource(const std::string &resourceId, bool cascade = false);

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
    std::unordered_set<std::string> getDependencies(const std::string &resourceId);

    /**
     * @brief Get a resource node from the graph
     * 
     * @param resourceId Resource identifier
     * @return Shared pointer to the node or nullptr if not found
     */
    std::shared_ptr<CoordinatedGraph<std::shared_ptr<Resource>>::Node> 
    getResourceNode(const std::string &resourceId, size_t timeoutMs = 100);

    /**
     * @brief Get all resource IDs
     * 
     * @return Vector of all resource IDs
     */
    std::vector<std::string> getAllResourceIds() const;

    /**
     * @brief Clear all resources
     */
    void clear();

private:
    // The graph of resource dependencies
    CoordinatedGraph<std::shared_ptr<Resource>> resourceGraph_;
    
    // Helper method for recursive removal
    bool removeResourceRecursive(const std::string &resourceId);
};

} // namespace Fabric