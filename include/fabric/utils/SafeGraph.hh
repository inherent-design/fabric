#pragma once

#include <algorithm>
#include <functional>
#include <mutex>
#include <shared_mutex>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <optional>
#include <stdexcept>
#include <string>
#include <queue>
#include <stack>
#include <chrono>
#include <thread>
#include "fabric/utils/TimeoutLock.hh"
#include "fabric/utils/Logging.hh"

namespace Fabric {
namespace Utils {

/**
 * @brief Exception thrown when a cycle is detected in the graph
 */
class GraphCycleException : public std::runtime_error {
public:
    explicit GraphCycleException(const std::string& message)
        : std::runtime_error(message) {}
};

/**
 * @brief A simplified, thread-safe graph implementation
 * 
 * This class provides a thread-safe graph implementation with clear separation
 * of concerns and robust error handling. It supports both node data access and
 * graph structure operations with appropriate locking.
 * 
 * @tparam NodeData Type of data stored in nodes
 * @tparam NodeKey Type of node identifier (default: std::string)
 */
template<typename NodeData, typename NodeKey = std::string>
class SafeGraph {
public:
    /**
     * @brief States used for graph traversal algorithms
     */
    enum class NodeState {
        Unvisited,
        Visiting,
        Visited
    };
    
    /**
     * @brief Node in the graph
     */
    struct Node {
        NodeData data;
        std::chrono::steady_clock::time_point lastAccessTime;
        
        explicit Node(NodeData data)
            : data(std::move(data)), 
              lastAccessTime(std::chrono::steady_clock::now()) {}
        
        /**
         * @brief Update the last access time
         */
        void touch() {
            lastAccessTime = std::chrono::steady_clock::now();
        }
    };
    
    /**
     * @brief Construct a new SafeGraph
     */
    SafeGraph() = default;
    
    /**
     * @brief Destructor
     */
    ~SafeGraph() = default;
    
    /**
     * @brief SafeGraph is not copyable
     */
    SafeGraph(const SafeGraph&) = delete;
    SafeGraph& operator=(const SafeGraph&) = delete;
    
    /**
     * @brief SafeGraph is movable
     */
    SafeGraph(SafeGraph&&) noexcept = default;
    SafeGraph& operator=(SafeGraph&&) noexcept = default;
    
    /**
     * @brief Add a node to the graph
     * 
     * @param key Node identifier
     * @param data Node data
     * @return true if node was added, false if a node with this key already exists
     */
    bool addNode(const NodeKey& key, NodeData data) {
        // Get a write lock on the graph
        auto lock = TimeoutLock<std::shared_mutex>::tryLockUnique(graphMutex_);
        if (!lock) {
            Logger::logWarning("Failed to acquire lock for adding node");
            return false;
        }
        
        // Check if the node already exists
        if (nodes_.find(key) != nodes_.end()) {
            return false;
        }
        
        // Add the node
        nodes_.emplace(key, Node(std::move(data)));
        
        // Initialize edge sets
        outEdges_[key] = std::unordered_set<NodeKey>();
        inEdges_[key] = std::unordered_set<NodeKey>();
        
        return true;
    }
    
    /**
     * @brief Remove a node from the graph
     * 
     * @param key Node identifier
     * @return true if node was removed, false if it didn't exist
     */
    bool removeNode(const NodeKey& key) {
        // Get a write lock on the graph
        auto lock = TimeoutLock<std::shared_mutex>::tryLockUnique(graphMutex_);
        if (!lock) {
            Logger::logWarning("Failed to acquire lock for removing node");
            return false;
        }
        
        // Check if the node exists
        if (nodes_.find(key) == nodes_.end()) {
            return false;
        }
        
        // Remove all edges connected to this node
        for (const auto& targetKey : outEdges_[key]) {
            inEdges_[targetKey].erase(key);
        }
        
        for (const auto& sourceKey : inEdges_[key]) {
            outEdges_[sourceKey].erase(key);
        }
        
        // Remove the node and its edge sets
        nodes_.erase(key);
        outEdges_.erase(key);
        inEdges_.erase(key);
        
        return true;
    }
    
    /**
     * @brief Check if a node exists
     * 
     * @param key Node identifier
     * @return true if the node exists, false otherwise
     */
    bool hasNode(const NodeKey& key) const {
        // Get a read lock on the graph
        auto lock = TimeoutLock<std::shared_mutex>::tryLockShared(graphMutex_);
        if (!lock) {
            Logger::logWarning("Failed to acquire lock for checking node existence");
            return false;
        }
        
        return nodes_.find(key) != nodes_.end();
    }
    
    /**
     * @brief Add a directed edge between two nodes
     * 
     * @param fromKey Source node key
     * @param toKey Target node key
     * @return true if the edge was added, false if it already exists or nodes don't exist
     * @throws GraphCycleException if adding this edge would create a cycle
     */
    bool addEdge(const NodeKey& fromKey, const NodeKey& toKey) {
        // Get a write lock on the graph
        auto lock = TimeoutLock<std::shared_mutex>::tryLockUnique(graphMutex_);
        if (!lock) {
            Logger::logWarning("Failed to acquire lock for adding edge");
            return false;
        }
        
        // Check if both nodes exist
        if (nodes_.find(fromKey) == nodes_.end() || nodes_.find(toKey) == nodes_.end()) {
            return false;
        }
        
        // Check if the edge already exists
        if (outEdges_[fromKey].find(toKey) != outEdges_[fromKey].end()) {
            return false;
        }
        
        // Add the edge
        outEdges_[fromKey].insert(toKey);
        inEdges_[toKey].insert(fromKey);
        
        // Check for cycles
        if (hasCycleFrom(toKey, fromKey)) {
            // Rollback the edge addition
            outEdges_[fromKey].erase(toKey);
            inEdges_[toKey].erase(fromKey);
            throw GraphCycleException("Adding this edge would create a cycle in the graph");
        }
        
        return true;
    }
    
    /**
     * @brief Remove a directed edge between two nodes
     * 
     * @param fromKey Source node key
     * @param toKey Target node key
     * @return true if the edge was removed, false if it didn't exist or nodes don't exist
     */
    bool removeEdge(const NodeKey& fromKey, const NodeKey& toKey) {
        // Get a write lock on the graph
        auto lock = TimeoutLock<std::shared_mutex>::tryLockUnique(graphMutex_);
        if (!lock) {
            Logger::logWarning("Failed to acquire lock for removing edge");
            return false;
        }
        
        // Check if both nodes exist
        if (nodes_.find(fromKey) == nodes_.end() || nodes_.find(toKey) == nodes_.end()) {
            return false;
        }
        
        // Check if the edge exists
        if (outEdges_[fromKey].find(toKey) == outEdges_[fromKey].end()) {
            return false;
        }
        
        // Remove the edge
        outEdges_[fromKey].erase(toKey);
        inEdges_[toKey].erase(fromKey);
        
        return true;
    }
    
    /**
     * @brief Check if an edge exists between two nodes
     * 
     * @param fromKey Source node key
     * @param toKey Target node key
     * @return true if the edge exists, false otherwise
     */
    bool hasEdge(const NodeKey& fromKey, const NodeKey& toKey) const {
        // Get a read lock on the graph
        auto lock = TimeoutLock<std::shared_mutex>::tryLockShared(graphMutex_);
        if (!lock) {
            Logger::logWarning("Failed to acquire lock for checking edge existence");
            return false;
        }
        
        // Check if both nodes exist
        if (nodes_.find(fromKey) == nodes_.end() || nodes_.find(toKey) == nodes_.end()) {
            return false;
        }
        
        // Check if the edge exists
        return outEdges_.at(fromKey).find(toKey) != outEdges_.at(fromKey).end();
    }
    
    /**
     * @brief Get all outgoing edges from a node
     * 
     * @param key Node identifier
     * @return Vector of target node keys, or empty vector if the node doesn't exist
     */
    std::vector<NodeKey> getOutEdges(const NodeKey& key) const {
        // Get a read lock on the graph
        auto lock = TimeoutLock<std::shared_mutex>::tryLockShared(graphMutex_);
        if (!lock) {
            Logger::logWarning("Failed to acquire lock for getting outgoing edges");
            return {};
        }
        
        // Check if the node exists
        auto it = outEdges_.find(key);
        if (it == outEdges_.end()) {
            return {};
        }
        
        // Copy the edges
        return std::vector<NodeKey>(it->second.begin(), it->second.end());
    }
    
    /**
     * @brief Get all incoming edges to a node
     * 
     * @param key Node identifier
     * @return Vector of source node keys, or empty vector if the node doesn't exist
     */
    std::vector<NodeKey> getInEdges(const NodeKey& key) const {
        // Get a read lock on the graph
        auto lock = TimeoutLock<std::shared_mutex>::tryLockShared(graphMutex_);
        if (!lock) {
            Logger::logWarning("Failed to acquire lock for getting incoming edges");
            return {};
        }
        
        // Check if the node exists
        auto it = inEdges_.find(key);
        if (it == inEdges_.end()) {
            return {};
        }
        
        // Copy the edges
        return std::vector<NodeKey>(it->second.begin(), it->second.end());
    }
    
    /**
     * @brief Perform a topological sort of the graph
     * 
     * @return Vector of node keys in topological order, or empty vector if the graph has cycles
     */
    std::vector<NodeKey> topologicalSort() const {
        // Make a copy of the graph structure under a read lock
        std::unordered_map<NodeKey, std::unordered_set<NodeKey>> localOutEdges;
        std::vector<NodeKey> localNodes;
        
        // Get a read lock on the graph
        auto lock = TimeoutLock<std::shared_mutex>::tryLockShared(graphMutex_);
        if (!lock) {
            Logger::logWarning("Failed to acquire lock for topological sort");
            return {};
        }
        
        // If the graph is empty, return an empty result
        if (nodes_.empty()) {
            return {};
        }
        
        // Copy the nodes and outgoing edges
        for (const auto& [key, _] : nodes_) {
            localNodes.push_back(key);
            
            auto edgeIt = outEdges_.find(key);
            if (edgeIt != outEdges_.end()) {
                localOutEdges[key] = edgeIt->second;
            } else {
                localOutEdges[key] = {};
            }
        }
        
        // Release the lock before performing the sort
        lock.reset();
        
        // Create data structures for the sort
        std::vector<NodeKey> result;
        std::unordered_map<NodeKey, bool> visited;
        std::unordered_map<NodeKey, bool> inProcess;
        
        // Helper function for DFS-based topological sort
        std::function<bool(const NodeKey&)> visit = [&](const NodeKey& key) {
            // If the node is currently being processed, we've found a cycle
            if (inProcess[key]) {
                return false;
            }
            
            // If the node has already been visited, skip it
            if (visited[key]) {
                return true;
            }
            
            // Mark the node as being processed
            inProcess[key] = true;
            
            // Visit all outgoing edges
            for (const auto& targetKey : localOutEdges[key]) {
                if (!visit(targetKey)) {
                    return false;
                }
            }
            
            // Mark the node as visited and add it to the result
            inProcess[key] = false;
            visited[key] = true;
            result.push_back(key);
            
            return true;
        };
        
        // Perform the topological sort
        for (const auto& key : localNodes) {
            if (!visited[key]) {
                if (!visit(key)) {
                    // Cycle detected
                    return {};
                }
            }
        }
        
        // Reverse the result to get topological order
        std::reverse(result.begin(), result.end());
        
        return result;
    }
    
    /**
     * @brief Check if the graph has any cycles
     * 
     * @return true if the graph has cycles, false otherwise
     */
    bool hasCycle() const {
        // Get a read lock on the graph
        auto lock = TimeoutLock<std::shared_mutex>::tryLockShared(graphMutex_);
        if (!lock) {
            Logger::logWarning("Failed to acquire lock for cycle detection");
            return false;
        }
        
        // If the graph is empty or has only one node, it can't have cycles
        if (nodes_.size() <= 1) {
            return false;
        }
        
        // Make a local copy of the node keys
        std::vector<NodeKey> localNodes;
        for (const auto& [key, _] : nodes_) {
            localNodes.push_back(key);
        }
        
        // Release the lock before the traversal
        lock.reset();
        
        // Check each node for cycles
        std::unordered_map<NodeKey, NodeState> visited;
        for (const auto& key : localNodes) {
            if (visited.find(key) == visited.end()) {
                if (hasCycleDFS(key, visited)) {
                    return true;
                }
            }
        }
        
        return false;
    }
    
    /**
     * @brief Traverse the graph in breadth-first order starting from a node
     * 
     * @param startKey Key of the starting node
     * @param visitFunc Function to call for each visited node
     */
    void bfs(const NodeKey& startKey, std::function<void(const NodeKey&, const NodeData&)> visitFunc) const {
        // Make local copies for the traversal
        std::unordered_map<NodeKey, std::unordered_set<NodeKey>> localOutEdges;
        
        // Get a read lock on the graph
        auto lock = TimeoutLock<std::shared_mutex>::tryLockShared(graphMutex_);
        if (!lock) {
            Logger::logWarning("Failed to acquire lock for BFS traversal");
            return;
        }
        
        // Check if the start node exists
        if (nodes_.find(startKey) == nodes_.end()) {
            return;
        }
        
        // Make a copy of the start node data
        NodeData startData = nodes_.at(startKey).data;
        
        // Get the outgoing edges for the start node
        if (outEdges_.find(startKey) != outEdges_.end()) {
            localOutEdges[startKey] = outEdges_.at(startKey);
        } else {
            localOutEdges[startKey] = {};
        }
        
        // Release the lock before the traversal
        lock.reset();
        
        // Visit the start node
        visitFunc(startKey, startData);
        
        // Initialize BFS
        std::queue<NodeKey> queue;
        std::unordered_set<NodeKey> visited{startKey};
        
        // Add the start node's neighbors to the queue
        for (const auto& neighborKey : localOutEdges[startKey]) {
            queue.push(neighborKey);
            visited.insert(neighborKey);
        }
        
        // Process the queue
        while (!queue.empty()) {
            NodeKey currentKey = queue.front();
            queue.pop();
            
            // Get the current node data under a read lock
            NodeData currentData;
            {
                auto nodeLock = TimeoutLock<std::shared_mutex>::tryLockShared(graphMutex_);
                if (!nodeLock) {
                    continue;
                }
                
                // Check if the node still exists
                auto nodeIt = nodes_.find(currentKey);
                if (nodeIt == nodes_.end()) {
                    continue;
                }
                
                // Copy the node data
                currentData = nodeIt->second.data;
                
                // Copy the outgoing edges
                if (outEdges_.find(currentKey) != outEdges_.end()) {
                    localOutEdges[currentKey] = outEdges_.at(currentKey);
                } else {
                    localOutEdges[currentKey] = {};
                }
            }
            
            // Visit the current node
            visitFunc(currentKey, currentData);
            
            // Add unvisited neighbors to the queue
            for (const auto& neighborKey : localOutEdges[currentKey]) {
                if (visited.find(neighborKey) == visited.end()) {
                    visited.insert(neighborKey);
                    queue.push(neighborKey);
                }
            }
        }
    }
    
    /**
     * @brief Traverse the graph in depth-first order starting from a node
     * 
     * @param startKey Key of the starting node
     * @param visitFunc Function to call for each visited node
     */
    void dfs(const NodeKey& startKey, std::function<void(const NodeKey&, const NodeData&)> visitFunc) const {
        // Make local copies for the traversal
        std::unordered_map<NodeKey, std::unordered_set<NodeKey>> localOutEdges;
        
        // Get a read lock on the graph
        auto lock = TimeoutLock<std::shared_mutex>::tryLockShared(graphMutex_);
        if (!lock) {
            Logger::logWarning("Failed to acquire lock for DFS traversal");
            return;
        }
        
        // Check if the start node exists
        if (nodes_.find(startKey) == nodes_.end()) {
            return;
        }
        
        // Make a copy of the start node data
        NodeData startData = nodes_.at(startKey).data;
        
        // Get the outgoing edges for the start node
        if (outEdges_.find(startKey) != outEdges_.end()) {
            localOutEdges[startKey] = outEdges_.at(startKey);
        } else {
            localOutEdges[startKey] = {};
        }
        
        // Release the lock before the traversal
        lock.reset();
        
        // Initialize DFS
        std::stack<NodeKey> stack;
        std::unordered_set<NodeKey> visited;
        
        // Push the start node
        stack.push(startKey);
        
        // Process the stack
        while (!stack.empty()) {
            NodeKey currentKey = stack.top();
            stack.pop();
            
            // Skip if already visited
            if (visited.find(currentKey) != visited.end()) {
                continue;
            }
            
            // Mark as visited
            visited.insert(currentKey);
            
            // Get the current node data under a read lock
            NodeData currentData;
            {
                auto nodeLock = TimeoutLock<std::shared_mutex>::tryLockShared(graphMutex_);
                if (!nodeLock) {
                    continue;
                }
                
                // Check if the node still exists
                auto nodeIt = nodes_.find(currentKey);
                if (nodeIt == nodes_.end()) {
                    continue;
                }
                
                // Copy the node data
                currentData = nodeIt->second.data;
                
                // Copy the outgoing edges
                if (outEdges_.find(currentKey) != outEdges_.end()) {
                    localOutEdges[currentKey] = outEdges_.at(currentKey);
                } else {
                    localOutEdges[currentKey] = {};
                }
            }
            
            // Visit the current node
            visitFunc(currentKey, currentData);
            
            // Push unvisited neighbors to the stack (in reverse order to maintain DFS order)
            std::vector<NodeKey> neighbors(localOutEdges[currentKey].begin(), localOutEdges[currentKey].end());
            for (auto it = neighbors.rbegin(); it != neighbors.rend(); ++it) {
                if (visited.find(*it) == visited.end()) {
                    stack.push(*it);
                }
            }
        }
    }
    
    /**
     * @brief Execute a function with a node's data, allowing modification
     * 
     * @param key Node identifier
     * @param func Function to execute with the node data
     * @return Result of the function if executed, std::nullopt if the node doesn't exist or lock acquisition failed
     */
    template<typename Func>
    auto withNodeData(const NodeKey& key, Func&& func) 
    -> std::optional<std::invoke_result_t<Func, NodeData&>> {
        using ResultType = std::invoke_result_t<Func, NodeData&>;
        
        // Get a read lock on the graph
        auto graphLock = TimeoutLock<std::shared_mutex>::tryLockShared(graphMutex_);
        if (!graphLock) {
            Logger::logWarning("Failed to acquire graph lock for node data access");
            return std::nullopt;
        }
        
        // Check if the node exists
        auto nodeIt = nodes_.find(key);
        if (nodeIt == nodes_.end()) {
            return std::nullopt;
        }
        
        // Get a node-specific lock
        std::unique_lock<std::shared_mutex> nodeLock(nodeMutex_);
        
        // Release the graph lock before operating on the node
        graphLock.reset();
        
        // Update the last access time
        nodeIt->second.touch();
        
        // Execute the function with the node data
        if constexpr (std::is_same_v<ResultType, void>) {
            func(nodeIt->second.data);
            return std::optional<ResultType>(std::in_place);
        } else {
            return std::make_optional(func(nodeIt->second.data));
        }
    }
    
    /**
     * @brief Execute a function with a node's data, not allowing modification
     * 
     * @param key Node identifier
     * @param func Function to execute with the node data
     * @return Result of the function if executed, std::nullopt if the node doesn't exist or lock acquisition failed
     */
    template<typename Func>
    auto withNodeDataConst(const NodeKey& key, Func&& func) 
    -> std::optional<std::invoke_result_t<Func, const NodeData&>> {
        using ResultType = std::invoke_result_t<Func, const NodeData&>;
        
        // Get a read lock on the graph
        auto graphLock = TimeoutLock<std::shared_mutex>::tryLockShared(graphMutex_);
        if (!graphLock) {
            Logger::logWarning("Failed to acquire graph lock for node data access");
            return std::nullopt;
        }
        
        // Check if the node exists
        auto nodeIt = nodes_.find(key);
        if (nodeIt == nodes_.end()) {
            return std::nullopt;
        }
        
        // Get a node-specific lock (shared for read-only access)
        std::shared_lock<std::shared_mutex> nodeLock(nodeMutex_);
        
        // Release the graph lock before operating on the node
        graphLock.reset();
        
        // Update the last access time
        nodeIt->second.touch();
        
        // Execute the function with the node data
        if constexpr (std::is_same_v<ResultType, void>) {
            func(nodeIt->second.data);
            return std::optional<ResultType>(std::in_place);
        } else {
            return std::make_optional(func(nodeIt->second.data));
        }
    }
    
    /**
     * @brief Get all node keys in the graph
     * 
     * @return Vector of node keys
     */
    std::vector<NodeKey> getAllNodeKeys() const {
        // Get a read lock on the graph
        auto lock = TimeoutLock<std::shared_mutex>::tryLockShared(graphMutex_);
        if (!lock) {
            Logger::logWarning("Failed to acquire lock for getting all node keys");
            return {};
        }
        
        // Copy the node keys
        std::vector<NodeKey> keys;
        keys.reserve(nodes_.size());
        
        for (const auto& [key, _] : nodes_) {
            keys.push_back(key);
        }
        
        return keys;
    }
    
    /**
     * @brief Get the number of nodes in the graph
     * 
     * @return Node count
     */
    size_t size() const {
        // Get a read lock on the graph
        auto lock = TimeoutLock<std::shared_mutex>::tryLockShared(graphMutex_);
        if (!lock) {
            Logger::logWarning("Failed to acquire lock for getting graph size");
            return 0;
        }
        
        return nodes_.size();
    }
    
    /**
     * @brief Check if the graph is empty
     * 
     * @return true if the graph has no nodes, false otherwise
     */
    bool empty() const {
        // Get a read lock on the graph
        auto lock = TimeoutLock<std::shared_mutex>::tryLockShared(graphMutex_);
        if (!lock) {
            Logger::logWarning("Failed to acquire lock for checking if graph is empty");
            return true; // Assume empty if lock fails
        }
        
        return nodes_.empty();
    }
    
    /**
     * @brief Clear all nodes and edges from the graph
     */
    void clear() {
        // Get a write lock on the graph
        auto lock = TimeoutLock<std::shared_mutex>::tryLockUnique(graphMutex_);
        if (!lock) {
            Logger::logWarning("Failed to acquire lock for clearing graph");
            return;
        }
        
        nodes_.clear();
        outEdges_.clear();
        inEdges_.clear();
    }
    
private:
    /**
     * @brief Check if there's a path from one node to another
     * 
     * This helper method is used for cycle detection when adding edges.
     * 
     * @param startKey Key of the starting node
     * @param targetKey Key of the target node
     * @return true if a path exists, false otherwise
     */
    bool hasCycleFrom(const NodeKey& startKey, const NodeKey& targetKey) const {
        std::unordered_set<NodeKey> visited;
        std::queue<NodeKey> queue;
        
        queue.push(startKey);
        visited.insert(startKey);
        
        while (!queue.empty()) {
            NodeKey currentKey = queue.front();
            queue.pop();
            
            // If we've reached the target, we've found a cycle
            if (currentKey == targetKey) {
                return true;
            }
            
            // Add all unvisited neighbors to the queue
            auto edgeIt = outEdges_.find(currentKey);
            if (edgeIt != outEdges_.end()) {
                for (const auto& neighborKey : edgeIt->second) {
                    if (visited.find(neighborKey) == visited.end()) {
                        visited.insert(neighborKey);
                        queue.push(neighborKey);
                    }
                }
            }
        }
        
        return false;
    }
    
    /**
     * @brief Helper method for cycle detection using DFS
     * 
     * @param key Current node key
     * @param visited Map of visited nodes and their states
     * @return true if a cycle was detected, false otherwise
     */
    bool hasCycleDFS(const NodeKey& key, std::unordered_map<NodeKey, NodeState>& visited) const {
        // Mark node as being visited
        visited[key] = NodeState::Visiting;
        
        // Check all outgoing edges
        std::unordered_set<NodeKey> neighbors;
        
        // Get the outgoing edges under a read lock
        {
            auto graphLock = TimeoutLock<std::shared_mutex>::tryLockShared(graphMutex_);
            if (!graphLock) {
                return false; // Assume no cycle if lock fails
            }
            
            auto edgeIt = outEdges_.find(key);
            if (edgeIt != outEdges_.end()) {
                neighbors = edgeIt->second;
            }
        }
        
        // Check each neighbor for cycles
        for (const auto& neighborKey : neighbors) {
            // Check if the neighbor exists in the visited map
            auto visitedIt = visited.find(neighborKey);
            
            if (visitedIt == visited.end()) {
                // Neighbor not visited yet, recurse
                if (hasCycleDFS(neighborKey, visited)) {
                    return true;
                }
            } else if (visitedIt->second == NodeState::Visiting) {
                // Neighbor is being visited, cycle detected
                return true;
            }
            // If neighbor is already visited, skip it
        }
        
        // Mark node as visited
        visited[key] = NodeState::Visited;
        
        return false;
    }
    
    // Graph nodes
    std::unordered_map<NodeKey, Node> nodes_;
    
    // Edge mappings for quick lookup
    std::unordered_map<NodeKey, std::unordered_set<NodeKey>> outEdges_;
    std::unordered_map<NodeKey, std::unordered_set<NodeKey>> inEdges_;
    
    // Mutex for graph structure modifications
    mutable std::shared_mutex graphMutex_;
    
    // Mutex for node data modifications
    mutable std::shared_mutex nodeMutex_;
};

} // namespace Utils
} // namespace Fabric