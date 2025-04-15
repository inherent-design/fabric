#pragma once

#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <shared_mutex>
#include <memory>
#include <functional>
#include <optional>
#include <atomic>
#include <condition_variable>
#include <queue>
#include <thread>
#include <string>
#include <stdexcept>
#include <cassert>
#include <algorithm>

namespace Fabric {

/**
 * @brief Exception thrown when a cycle is detected in the graph
 */
class CycleDetectedException : public std::runtime_error {
public:
    explicit CycleDetectedException(const std::string& message)
        : std::runtime_error(message) {}
};

/**
 * @brief A thread-safe directed graph implementation with fine-grained locking
 * 
 * This graph implementation is designed for concurrent access with node-level
 * locking for maximum parallelism. It supports dependency tracking, cycle detection,
 * and topological sorting, making it suitable for resource management, task scheduling,
 * and other dependency-based systems.
 * 
 * @tparam T Type of data stored in graph nodes
 * @tparam KeyType Type used as node identifier (default: std::string)
 */
template <typename T, typename KeyType = std::string>
class ConcurrentGraph {
public:
    /**
     * @brief Node states used for traversal algorithms
     */
    enum class NodeState {
        Unvisited,
        Visiting,
        Visited
    };

    /**
     * @brief A node in the graph
     * 
     * Each node has its own lock for fine-grained concurrency control.
     */
    class Node {
    public:
        /**
         * @brief Construct a node with the given data
         * 
         * @param key Node identifier
         * @param data Node data
         */
        Node(KeyType key, T data)
            : key_(std::move(key)), data_(std::move(data)), lastAccessTime_(std::chrono::steady_clock::now()) {}

        /**
         * @brief Get the node's key
         * 
         * @return Node key
         */
        const KeyType& getKey() const { return key_; }

        /**
         * @brief Get the node's data (const version)
         * 
         * @return Reference to the data
         */
        const T& getData() const {
            std::shared_lock<std::shared_mutex> lock(mutex_);
            return data_;
        }

        /**
         * @brief Get the node's data (mutable version)
         * 
         * @return Reference to the data
         */
        T& getData() {
            std::unique_lock<std::shared_mutex> lock(mutex_);
            lastAccessTime_ = std::chrono::steady_clock::now();
            return data_;
        }

        /**
         * @brief Set the node's data
         * 
         * @param data New data
         */
        void setData(T data) {
            std::unique_lock<std::shared_mutex> lock(mutex_);
            data_ = std::move(data);
            lastAccessTime_ = std::chrono::steady_clock::now();
        }

        /**
         * @brief Get the node's last access time
         * 
         * @return Last access time
         */
        std::chrono::steady_clock::time_point getLastAccessTime() const {
            std::shared_lock<std::shared_mutex> lock(mutex_);
            return lastAccessTime_;
        }

        /**
         * @brief Update the last access time to now
         */
        void touch() {
            std::unique_lock<std::shared_mutex> lock(mutex_);
            lastAccessTime_ = std::chrono::steady_clock::now();
        }

        /**
         * @brief Lock the node for exclusive access
         * 
         * @return Lock object that will be released when it goes out of scope
         */
        std::unique_lock<std::shared_mutex> lockExclusive() {
            return std::unique_lock<std::shared_mutex>(mutex_);
        }

        /**
         * @brief Lock the node for shared access
         * 
         * @return Lock object that will be released when it goes out of scope
         */
        std::shared_lock<std::shared_mutex> lockShared() {
            return std::shared_lock<std::shared_mutex>(mutex_);
        }

    private:
        KeyType key_;
        T data_;
        std::chrono::steady_clock::time_point lastAccessTime_;
        mutable std::shared_mutex mutex_;
    };

    ConcurrentGraph() = default;
    ~ConcurrentGraph() = default;

    /**
     * @brief Add a node to the graph
     * 
     * @param key Node identifier
     * @param data Node data
     * @return true if node was added, false if a node with this key already exists
     */
    bool addNode(const KeyType& key, T data) {
        std::unique_lock<std::shared_mutex> lock(graphMutex_);
        
        if (nodes_.find(key) != nodes_.end()) {
            return false;
        }
        
        auto node = std::make_shared<Node>(key, std::move(data));
        nodes_[key] = node;
        outEdges_[key] = std::unordered_set<KeyType>();
        inEdges_[key] = std::unordered_set<KeyType>();
        
        return true;
    }

    /**
     * @brief Remove a node from the graph
     * 
     * @param key Node identifier
     * @return true if node was removed, false if it didn't exist
     */
    bool removeNode(const KeyType& key) {
        std::unique_lock<std::shared_mutex> lock(graphMutex_);
        
        if (nodes_.find(key) == nodes_.end()) {
            return false;
        }
        
        // Remove all edges connected to this node
        for (const auto& target : outEdges_[key]) {
            inEdges_[target].erase(key);
        }
        
        for (const auto& source : inEdges_[key]) {
            outEdges_[source].erase(key);
        }
        
        // Remove the node
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
    bool hasNode(const KeyType& key) const {
        std::shared_lock<std::shared_mutex> lock(graphMutex_);
        return nodes_.find(key) != nodes_.end();
    }

    /**
     * @brief Get a node by key
     * 
     * @param key Node identifier
     * @return Shared pointer to the node or nullptr if not found
     */
    std::shared_ptr<Node> getNode(const KeyType& key) const {
        std::shared_lock<std::shared_mutex> lock(graphMutex_);
        auto it = nodes_.find(key);
        if (it != nodes_.end()) {
            return it->second;
        }
        return nullptr;
    }

    /**
     * @brief Add a directed edge between two nodes
     * 
     * @param fromKey Source node key
     * @param toKey Target node key
     * @param detectCycles Whether to check for cycles after adding the edge
     * @return true if the edge was added, false if it already exists or nodes don't exist
     * @throws CycleDetectedException if adding this edge would create a cycle and detectCycles is true
     */
    bool addEdge(const KeyType& fromKey, const KeyType& toKey, bool detectCycles = true) {
        std::unique_lock<std::shared_mutex> lock(graphMutex_);
        
        if (nodes_.find(fromKey) == nodes_.end() || nodes_.find(toKey) == nodes_.end()) {
            return false;
        }
        
        if (outEdges_[fromKey].find(toKey) != outEdges_[fromKey].end()) {
            return false;  // Edge already exists
        }
        
        outEdges_[fromKey].insert(toKey);
        inEdges_[toKey].insert(fromKey);
        
        if (detectCycles && hasCycle()) {
            // Rollback the edge addition
            outEdges_[fromKey].erase(toKey);
            inEdges_[toKey].erase(fromKey);
            throw CycleDetectedException("Adding this edge would create a cycle in the graph");
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
    bool removeEdge(const KeyType& fromKey, const KeyType& toKey) {
        std::unique_lock<std::shared_mutex> lock(graphMutex_);
        
        if (nodes_.find(fromKey) == nodes_.end() || nodes_.find(toKey) == nodes_.end()) {
            return false;
        }
        
        if (outEdges_[fromKey].find(toKey) == outEdges_[fromKey].end()) {
            return false;  // Edge doesn't exist
        }
        
        outEdges_[fromKey].erase(toKey);
        inEdges_[toKey].erase(fromKey);
        
        return true;
    }

    /**
     * @brief Check if an edge exists
     * 
     * @param fromKey Source node key
     * @param toKey Target node key
     * @return true if the edge exists, false otherwise
     */
    bool hasEdge(const KeyType& fromKey, const KeyType& toKey) const {
        std::shared_lock<std::shared_mutex> lock(graphMutex_);
        
        if (nodes_.find(fromKey) == nodes_.end() || nodes_.find(toKey) == nodes_.end()) {
            return false;
        }
        
        return outEdges_.at(fromKey).find(toKey) != outEdges_.at(fromKey).end();
    }

    /**
     * @brief Get all outgoing edges from a node
     * 
     * @param key Node identifier
     * @return Set of target node keys or empty set if node doesn't exist
     */
    std::unordered_set<KeyType> getOutEdges(const KeyType& key) const {
        std::shared_lock<std::shared_mutex> lock(graphMutex_);
        
        if (outEdges_.find(key) == outEdges_.end()) {
            return {};
        }
        
        return outEdges_.at(key);
    }

    /**
     * @brief Get all incoming edges to a node
     * 
     * @param key Node identifier
     * @return Set of source node keys or empty set if node doesn't exist
     */
    std::unordered_set<KeyType> getInEdges(const KeyType& key) const {
        std::shared_lock<std::shared_mutex> lock(graphMutex_);
        
        if (inEdges_.find(key) == inEdges_.end()) {
            return {};
        }
        
        return inEdges_.at(key);
    }

    /**
     * @brief Check if the graph has any cycles
     * 
     * @return true if the graph has cycles, false otherwise
     */
    bool hasCycle() const {
        std::shared_lock<std::shared_mutex> lock(graphMutex_);
        
        // If the graph is empty or has only one node, it can't have cycles
        if (nodes_.size() <= 1) {
            return false;
        }
        
        std::unordered_map<KeyType, NodeState> visited;
        
        for (const auto& node : nodes_) {
            if (visited.find(node.first) == visited.end()) {
                if (hasCycleInternal(node.first, visited)) {
                    return true;
                }
            }
        }
        
        return false;
    }

    /**
     * @brief Perform a topological sort of the graph
     * 
     * @return Vector of node keys in topological order or empty vector if the graph has cycles
     */
    std::vector<KeyType> topologicalSort() const {
        std::shared_lock<std::shared_mutex> lock(graphMutex_);
        
        // If the graph is empty, return an empty result
        if (nodes_.empty()) {
            return {};
        }
        
        std::vector<KeyType> result;
        std::unordered_map<KeyType, bool> visited;
        std::unordered_map<KeyType, bool> inProcess;
        
        std::function<bool(const KeyType&)> visit = [&](const KeyType& key) {
            if (inProcess[key]) {
                return false;  // Cycle detected
            }
            
            if (visited[key]) {
                return true;
            }
            
            inProcess[key] = true;
            
            // Safely get neighbors
            auto edgeIt = outEdges_.find(key);
            if (edgeIt != outEdges_.end()) {
                for (const auto& neighbor : edgeIt->second) {
                    // Check if neighbor exists
                    if (nodes_.find(neighbor) == nodes_.end()) {
                        continue;  // Skip non-existent nodes
                    }
                    
                    if (!visit(neighbor)) {
                        return false;
                    }
                }
            }
            
            inProcess[key] = false;
            visited[key] = true;
            result.push_back(key);
            
            return true;
        };
        
        for (const auto& node : nodes_) {
            if (!visited[node.first]) {
                if (!visit(node.first)) {
                    return {};  // Cycle detected
                }
            }
        }
        
        std::reverse(result.begin(), result.end());
        return result;
    }

    /**
     * @brief Traverse the graph in breadth-first order starting from a node
     * 
     * @param startKey Key of the starting node
     * @param visitFunc Function to call for each visited node
     */
    void bfs(const KeyType& startKey, std::function<void(const KeyType&, const T&)> visitFunc) const {
        // First collect all the information needed for traversal under consistent locks
        // to avoid holding locks during callbacks
        struct NodeWithData {
            KeyType key;
            T data;
        };
        
        std::queue<KeyType> queue;
        std::unordered_set<KeyType> visited;
        std::vector<NodeWithData> nodesToVisit;
        
        // Initial setup with the start node
        {
            std::shared_lock<std::shared_mutex> lock(graphMutex_);
            auto nodeIt = nodes_.find(startKey);
            if (nodeIt == nodes_.end()) {
                return;  // Start node doesn't exist
            }
            
            auto startNode = nodeIt->second;
            auto startNodeLock = startNode->lockShared();
            
            // Copy the data to avoid holding the lock during callback
            nodesToVisit.push_back({startKey, startNode->getData()});
            
            // Setup traversal variables
            visited.insert(startKey);
            queue.push(startKey);
        }
        
        // Visit the start node first
        visitFunc(nodesToVisit[0].key, nodesToVisit[0].data);
        nodesToVisit.clear();
        
        // BFS main loop - fully collect each level before visiting
        while (!queue.empty()) {
            // Collect the current level of nodes to visit
            size_t levelSize = queue.size();
            for (size_t i = 0; i < levelSize; ++i) {
                KeyType currentKey = queue.front();
                queue.pop();
                
                // Get outgoing edges with lock
                std::unordered_set<KeyType> neighbors;
                {
                    std::shared_lock<std::shared_mutex> lock(graphMutex_);
                    auto edgeIt = outEdges_.find(currentKey);
                    if (edgeIt != outEdges_.end()) {
                        neighbors = edgeIt->second; // Make a copy while locked
                    }
                }
                
                // For each neighbor, collect data under lock and prepare for callbacks
                for (const auto& neighborKey : neighbors) {
                    // Skip already visited nodes
                    if (visited.find(neighborKey) != visited.end()) {
                        continue;
                    }
                    
                    // Mark as visited and add to queue
                    visited.insert(neighborKey);
                    queue.push(neighborKey);
                    
                    // Get node data under lock
                    {
                        std::shared_lock<std::shared_mutex> lock(graphMutex_);
                        auto nodeIt = nodes_.find(neighborKey);
                        if (nodeIt != nodes_.end()) {
                            auto node = nodeIt->second;
                            auto nodeLock = node->lockShared();
                            // Copy the data to avoid holding lock during callback
                            nodesToVisit.push_back({neighborKey, node->getData()});
                        }
                    }
                }
            }
            
            // Process all nodes at this level without holding any locks
            for (const auto& nodeWithData : nodesToVisit) {
                visitFunc(nodeWithData.key, nodeWithData.data);
            }
            nodesToVisit.clear();
        }
    }

    /**
     * @brief Traverse the graph in depth-first order starting from a node
     * 
     * @param startKey Key of the starting node
     * @param visitFunc Function to call for each visited node
     */
    void dfs(const KeyType& startKey, std::function<void(const KeyType&, const T&)> visitFunc) const {
        // Use a non-recursive implementation to avoid stack overflow for large graphs
        // and collect data before making callbacks to avoid holding locks during callbacks
        struct NodeWithData {
            KeyType key;
            T data;
        };
        
        // Keep track of visited nodes to avoid cycles
        std::unordered_set<KeyType> visited;
        
        // Use an explicit stack for non-recursive DFS
        struct StackEntry {
            KeyType key;
            std::vector<KeyType> neighbors;
            size_t nextNeighborIndex;
        };
        
        std::vector<StackEntry> stack;
        std::vector<NodeWithData> nodesToVisit;
        
        // Initial setup with the start node
        {
            std::shared_lock<std::shared_mutex> lock(graphMutex_);
            auto nodeIt = nodes_.find(startKey);
            if (nodeIt == nodes_.end()) {
                return;  // Start node doesn't exist
            }
            
            // Get start node data
            auto startNode = nodeIt->second;
            auto startNodeLock = startNode->lockShared();
            nodesToVisit.push_back({startKey, startNode->getData()});
            
            // Mark start node as visited
            visited.insert(startKey);
            
            // Get outgoing edges
            std::vector<KeyType> neighbors;
            auto edgeIt = outEdges_.find(startKey);
            if (edgeIt != outEdges_.end()) {
                for (const auto& neighborKey : edgeIt->second) {
                    neighbors.push_back(neighborKey);
                }
            }
            
            // Push to stack if there are neighbors
            if (!neighbors.empty()) {
                stack.push_back({startKey, std::move(neighbors), 0});
            }
        }
        
        // Visit the start node
        visitFunc(nodesToVisit[0].key, nodesToVisit[0].data);
        nodesToVisit.clear();
        
        // Non-recursive DFS using an explicit stack
        while (!stack.empty()) {
            // Get the top stack entry
            auto& entry = stack.back();
            
            // If all neighbors have been processed, pop the stack
            if (entry.nextNeighborIndex >= entry.neighbors.size()) {
                stack.pop_back();
                continue;
            }
            
            // Get the next neighbor to process
            KeyType neighborKey = entry.neighbors[entry.nextNeighborIndex++];
            
            // Skip if already visited
            if (visited.find(neighborKey) != visited.end()) {
                continue;
            }
            
            // Mark as visited and get data
            visited.insert(neighborKey);
            
            // Get node data under lock
            bool hasNeighbors = false;
            std::vector<KeyType> nextNeighbors;
            {
                std::shared_lock<std::shared_mutex> lock(graphMutex_);
                auto nodeIt = nodes_.find(neighborKey);
                if (nodeIt == nodes_.end()) {
                    continue;  // Node doesn't exist
                }
                
                // Get node data
                auto node = nodeIt->second;
                auto nodeLock = node->lockShared();
                nodesToVisit.push_back({neighborKey, node->getData()});
                
                // Get outgoing edges
                auto edgeIt = outEdges_.find(neighborKey);
                if (edgeIt != outEdges_.end() && !edgeIt->second.empty()) {
                    hasNeighbors = true;
                    for (const auto& nextKey : edgeIt->second) {
                        nextNeighbors.push_back(nextKey);
                    }
                }
            }
            
            // Visit the node
            visitFunc(nodesToVisit.back().key, nodesToVisit.back().data);
            nodesToVisit.clear();
            
            // Push to stack if there are neighbors
            if (hasNeighbors) {
                stack.push_back({neighborKey, std::move(nextNeighbors), 0});
            }
        }
    }

    /**
     * @brief Process nodes in dependency order, ensuring dependencies are processed before dependents
     * 
     * @param processFunc Function to call for each node
     * @return true if all nodes were processed, false if a cycle was detected
     */
    bool processDependencyOrder(std::function<void(const KeyType&, T&)> processFunc) {
        std::shared_lock<std::shared_mutex> graphLock(graphMutex_);
        
        // First, compute a topological sort
        auto sortedNodes = topologicalSort();
        if (sortedNodes.empty() && !nodes_.empty()) {
            return false;  // Cycle detected
        }
        
        // Process nodes in topological order
        for (const auto& key : sortedNodes) {
            auto nodePtr = getNode(key);
            if (nodePtr) {
                auto nodeLock = nodePtr->lockExclusive();
                processFunc(key, nodePtr->getData());
            }
        }
        
        return true;
    }

    /**
     * @brief Get all node keys in the graph
     * 
     * @return Vector of all node keys
     */
    std::vector<KeyType> getAllNodes() const {
        std::shared_lock<std::shared_mutex> lock(graphMutex_);
        
        std::vector<KeyType> keys;
        keys.reserve(nodes_.size());
        
        for (const auto& node : nodes_) {
            keys.push_back(node.first);
        }
        
        return keys;
    }

    /**
     * @brief Get the number of nodes in the graph
     * 
     * @return Node count
     */
    size_t size() const {
        std::shared_lock<std::shared_mutex> lock(graphMutex_);
        return nodes_.size();
    }

    /**
     * @brief Check if the graph is empty
     * 
     * @return true if the graph has no nodes, false otherwise
     */
    bool empty() const {
        std::shared_lock<std::shared_mutex> lock(graphMutex_);
        return nodes_.empty();
    }

    /**
     * @brief Clear all nodes and edges from the graph
     */
    void clear() {
        std::unique_lock<std::shared_mutex> lock(graphMutex_);
        nodes_.clear();
        outEdges_.clear();
        inEdges_.clear();
    }

private:
    /**
     * @brief Internal helper method for cycle detection
     * 
     * @param key Current node key
     * @param visited Map of visited nodes and their states
     * @return true if a cycle was detected, false otherwise
     */
    bool hasCycleInternal(const KeyType& key, std::unordered_map<KeyType, NodeState>& visited) const {
        visited[key] = NodeState::Visiting;
        
        // Safe access to outEdges
        auto it = outEdges_.find(key);
        if (it == outEdges_.end()) {
            // Key not found in outEdges, mark as visited and return
            visited[key] = NodeState::Visited;
            return false;
        }
        
        const auto& neighbors = it->second;
        for (const auto& neighbor : neighbors) {
            // Check if the neighbor exists in the nodes map
            if (nodes_.find(neighbor) == nodes_.end()) {
                continue;  // Skip non-existent nodes
            }
            
            if (visited.find(neighbor) == visited.end()) {
                if (hasCycleInternal(neighbor, visited)) {
                    return true;
                }
            } else if (visited[neighbor] == NodeState::Visiting) {
                return true;  // Cycle detected
            }
        }
        
        visited[key] = NodeState::Visited;
        return false;
    }

    mutable std::shared_mutex graphMutex_;
    std::unordered_map<KeyType, std::shared_ptr<Node>> nodes_;
    std::unordered_map<KeyType, std::unordered_set<KeyType>> outEdges_;
    std::unordered_map<KeyType, std::unordered_set<KeyType>> inEdges_;
};

} // namespace Fabric