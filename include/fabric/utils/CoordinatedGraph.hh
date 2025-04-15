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
#include <chrono>
#include <future>
#include <stack>

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
 * @brief Exception thrown when a lock cannot be acquired
 */
class LockAcquisitionException : public std::runtime_error {
public:
    explicit LockAcquisitionException(const std::string& message)
        : std::runtime_error(message) {}
};

/**
 * @brief A thread-safe directed graph implementation with intentional locking
 * 
 * This graph implementation is designed for concurrent access with node-level
 * locking for maximum parallelism, while providing awareness of different
 * lock types and their intentions. It enables high-priority graph operations
 * to coordinate with lower-priority node operations.
 * 
 * Key Features:
 * - Intent-based locking (read, write, structure modification)
 * - Lock hierarchy to prevent deadlocks (graph lock → node locks, never reverse)
 * - Non-blocking operations with try_lock to avoid indefinite waits
 * - Thread-safe node access through explicit locking mechanisms
 * - Awareness propagation between locks of different levels
 * 
 * @tparam T Type of data stored in graph nodes
 * @tparam KeyType Type used as node identifier (default: std::string)
 */
template <typename T, typename KeyType = std::string>
class CoordinatedGraph {
public:
    /**
     * @brief Lock intent type to specify the purpose of a lock
     */
    enum class LockIntent {
        Read,              // Intent to read without modification
        NodeModify,        // Intent to modify node data only
        GraphStructure,    // Intent to modify graph structure (highest priority)
    };

    /**
     * @brief Status of a lock for notification callbacks
     */
    enum class LockStatus {
        Acquired,          // Lock has been acquired
        Released,          // Lock has been released
        Preempted,         // Lock has been preempted by higher priority
        BackgroundWait,    // Lock is temporarily waiting for structural changes
        Failed             // Lock acquisition failed
    };

    /**
     * @brief Node states used for traversal algorithms
     */
    enum class NodeState {
        Unvisited,
        Visiting,
        Visited
    };

    // Forward declaration of lock handles
    class NodeLockHandle;
    class GraphLockHandle;

    /**
     * @brief A node in the graph
     * 
     * Each node has its own lock for fine-grained concurrency control.
     */
    class Node {
    public:
        using LockCallback = std::function<void(LockStatus)>;

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
         * @brief Try to acquire a lock with specified intent and timeout
         * 
         * @param intent Purpose of the lock
         * @param timeoutMs Timeout in milliseconds (default: 100ms)
         * @param callback Function to call when lock status changes
         * @return A lock handle or nullptr if acquisition failed
         */
        std::unique_ptr<NodeLockHandle> tryLock(
            LockIntent intent, 
            size_t timeoutMs = 100,
            LockCallback callback = nullptr
        ) {
            // This implementation is inlined to avoid the helper function issues
            using namespace std::chrono;
            
            // For read locks
            if (intent == LockIntent::Read) {
                std::shared_lock<std::shared_mutex> lock(mutex_, std::try_to_lock);
                if (lock.owns_lock()) {
                    return std::make_unique<NodeLockHandle>(
                        this, 
                        std::move(lock),
                        intent,
                        callback
                    );
                }
                
                // If immediate acquisition failed, try with timeout
                auto start = steady_clock::now();
                while (true) {
                    lock = std::shared_lock<std::shared_mutex>(mutex_, std::try_to_lock);
                    if (lock.owns_lock()) {
                        return std::make_unique<NodeLockHandle>(
                            this, 
                            std::move(lock),
                            intent,
                            callback
                        );
                    }
                    
                    if (duration_cast<milliseconds>(steady_clock::now() - start).count() >= timeoutMs) {
                        return nullptr;
                    }
                    
                    std::this_thread::sleep_for(milliseconds(1));
                }
            } 
            // For write locks
            else {
                std::unique_lock<std::shared_mutex> lock(mutex_, std::try_to_lock);
                if (lock.owns_lock()) {
                    return std::make_unique<NodeLockHandle>(
                        this, 
                        std::move(lock),
                        intent,
                        callback
                    );
                }
                
                // If immediate acquisition failed, try with timeout
                auto start = steady_clock::now();
                while (true) {
                    lock = std::unique_lock<std::shared_mutex>(mutex_, std::try_to_lock);
                    if (lock.owns_lock()) {
                        return std::make_unique<NodeLockHandle>(
                            this, 
                            std::move(lock),
                            intent,
                            callback
                        );
                    }
                    
                    if (duration_cast<milliseconds>(steady_clock::now() - start).count() >= timeoutMs) {
                        return nullptr;
                    }
                    
                    std::this_thread::sleep_for(milliseconds(1));
                }
            }
        }

    private:
        friend class CoordinatedGraph;
        friend class NodeLockHandle;

        KeyType key_;
        T data_;
        std::chrono::steady_clock::time_point lastAccessTime_;
        mutable std::shared_mutex mutex_;
        std::vector<std::pair<LockIntent, LockCallback>> activeCallbacks_;
        std::mutex callbackMutex_;
        
        // Method to notify all lock holders with callbacks
        void notifyLockHolders(LockStatus status) {
            std::lock_guard<std::mutex> lock(callbackMutex_);
            for (auto& [intent, callback] : activeCallbacks_) {
                if (callback) {
                    callback(status);
                }
            }
        }
        
        // Register a callback for this lock
        void registerCallback(LockIntent intent, LockCallback callback) {
            if (!callback) return;
            std::lock_guard<std::mutex> lock(callbackMutex_);
            activeCallbacks_.push_back({intent, callback});
        }
        
        // Remove a callback
        void removeCallback(LockIntent intent, LockCallback callback) {
            if (!callback) return;
            std::lock_guard<std::mutex> lock(callbackMutex_);
            // Since we can't directly compare std::function objects, use a simpler approach
            // Just remove all callbacks with the same intent (this works because we register/remove
            // specific intents that match)
            auto it = activeCallbacks_.begin();
            while (it != activeCallbacks_.end()) {
                if (it->first == intent) {
                    it = activeCallbacks_.erase(it);
                } else {
                    ++it;
                }
            }
        }
    };

    /**
     * @brief A handle for a node lock that automatically releases on destruction
     */
    class NodeLockHandle {
    public:
        /**
         * @brief Constructor for a read lock
         */
        NodeLockHandle(
            Node* node, 
            std::shared_lock<std::shared_mutex> lock,
            LockIntent intent,
            typename Node::LockCallback callback
        ) : node_(node), 
            readLock_(std::move(lock)), 
            writeLock_(),
            isReadLock_(true),
            intent_(intent),
            callback_(callback) {
            if (node_ && callback_) {
                node_->registerCallback(intent_, callback_);
            }
        }
        
        /**
         * @brief Constructor for a write lock
         */
        NodeLockHandle(
            Node* node, 
            std::unique_lock<std::shared_mutex> lock,
            LockIntent intent,
            typename Node::LockCallback callback
        ) : node_(node), 
            readLock_(), 
            writeLock_(std::move(lock)),
            isReadLock_(false),
            intent_(intent),
            callback_(callback) {
            if (node_ && callback_) {
                node_->registerCallback(intent_, callback_);
            }
        }
        
        /**
         * @brief Destructor that releases the lock
         */
        ~NodeLockHandle() {
            if (node_ && callback_) {
                node_->removeCallback(intent_, callback_);
            }
        }
        
        /**
         * @brief Check if the lock is currently held
         * 
         * @return true if the lock is held, false otherwise
         */
        bool isLocked() const {
            return isReadLock_ ? readLock_.owns_lock() : writeLock_.owns_lock();
        }
        
        /**
         * @brief Release the lock early (before destruction)
         */
        void release() {
            if (isReadLock_) {
                readLock_.unlock();
            } else {
                writeLock_.unlock();
            }
            
            if (node_ && callback_) {
                node_->removeCallback(intent_, callback_);
                callback_ = nullptr;
            }
        }
        
        /**
         * @brief Get the node this lock is for
         * 
         * @return Pointer to the node
         */
        Node* getNode() const {
            return node_;
        }
        
        /**
         * @brief Get the intent of this lock
         * 
         * @return Intent of the lock
         */
        LockIntent getIntent() const {
            return intent_;
        }
        
    private:
        Node* node_;
        std::shared_lock<std::shared_mutex> readLock_;
        std::unique_lock<std::shared_mutex> writeLock_;
        bool isReadLock_;
        LockIntent intent_;
        typename Node::LockCallback callback_;
    };

    /**
     * @brief A handle for a graph lock that automatically releases on destruction
     */
    class GraphLockHandle {
    public:
        /**
         * @brief Constructor for a read lock
         */
        GraphLockHandle(
            CoordinatedGraph* graph, 
            std::shared_lock<std::shared_mutex> lock,
            LockIntent intent
        ) : graph_(graph), 
            readLock_(std::move(lock)), 
            writeLock_(),
            isReadLock_(true),
            intent_(intent) {}
        
        /**
         * @brief Constructor for a write lock
         */
        GraphLockHandle(
            CoordinatedGraph* graph, 
            std::unique_lock<std::shared_mutex> lock,
            LockIntent intent
        ) : graph_(graph), 
            readLock_(), 
            writeLock_(std::move(lock)),
            isReadLock_(false),
            intent_(intent) {}
        
        /**
         * @brief Destructor that releases the lock
         */
        ~GraphLockHandle() {
            release();
        }
        
        /**
         * @brief Check if the lock is currently held
         * 
         * @return true if the lock is held, false otherwise
         */
        bool isLocked() const {
            return isReadLock_ ? readLock_.owns_lock() : writeLock_.owns_lock();
        }
        
        /**
         * @brief Release the lock early (before destruction)
         */
        void release() {
            if (isReadLock_) {
                if (readLock_.owns_lock()) {
                    readLock_.unlock();
                    if (graph_) {
                        graph_->onGraphLockReleased(intent_);
                    }
                }
            } else {
                if (writeLock_.owns_lock()) {
                    writeLock_.unlock();
                    if (graph_) {
                        graph_->onGraphLockReleased(intent_);
                    }
                }
            }
        }
        
        /**
         * @brief Get the intent of this lock
         * 
         * @return Intent of the lock
         */
        LockIntent getIntent() const {
            return intent_;
        }
        
    private:
        CoordinatedGraph* graph_;
        std::shared_lock<std::shared_mutex> readLock_;
        std::unique_lock<std::shared_mutex> writeLock_;
        bool isReadLock_;
        LockIntent intent_;
    };

    CoordinatedGraph() = default;
    ~CoordinatedGraph() = default;

    /**
     * @brief Add a node to the graph
     * 
     * @param key Node identifier
     * @param data Node data
     * @return true if node was added, false if a node with this key already exists
     */
    bool addNode(const KeyType& key, T data) {
        auto lock = lockGraph(LockIntent::GraphStructure);
        if (!lock || !lock->isLocked()) {
            throw LockAcquisitionException("Failed to acquire graph lock for node addition");
        }
        
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
        auto lock = lockGraph(LockIntent::GraphStructure);
        if (!lock || !lock->isLocked()) {
            throw LockAcquisitionException("Failed to acquire graph lock for node removal");
        }
        
        if (nodes_.find(key) == nodes_.end()) {
            return false;
        }
        
        // Notify node lock holders about the impending removal
        auto nodePtr = nodes_[key];
        if (nodePtr) {
            nodePtr->notifyLockHolders(LockStatus::Preempted);
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
        
        // Signal that a node was removed (for anyone who needs to know)
        onNodeRemoved(key);
        
        return true;
    }

    /**
     * @brief Check if a node exists
     * 
     * @param key Node identifier
     * @return true if the node exists, false otherwise
     */
    bool hasNode(const KeyType& key) const {
        auto lock = lockGraph(LockIntent::Read);
        if (!lock || !lock->isLocked()) {
            throw LockAcquisitionException("Failed to acquire graph lock for node check");
        }
        
        return nodes_.find(key) != nodes_.end();
    }

    /**
     * @brief Get a node by key with timeout protection
     * 
     * @param key Node identifier
     * @param timeoutMs Timeout in milliseconds (default: 100ms)
     * @return Shared pointer to the node or nullptr if not found or timed out
     */
    std::shared_ptr<Node> getNode(const KeyType& key, size_t timeoutMs = 100) const {
        auto lock = lockGraph(LockIntent::Read, timeoutMs);
        if (!lock || !lock->isLocked()) {
            // Return nullptr instead of throwing if we couldn't acquire the lock
            return nullptr;
        }
        
        auto it = nodes_.find(key);
        return (it != nodes_.end()) ? it->second : nullptr;
    }

    /**
     * @brief Try to lock a specific node with an explicit intent
     * 
     * @param key Node identifier
     * @param intent Purpose of the lock
     * @param forWrite Whether to acquire a write lock
     * @param timeoutMs Timeout in milliseconds (default: 100ms)
     * @param callback Function to call when lock status changes
     * @return A lock handle or nullptr if acquisition failed
     */
    std::unique_ptr<NodeLockHandle> tryLockNode(
        const KeyType& key, 
        LockIntent intent, 
        bool forWrite = false,
        size_t timeoutMs = 100,
        typename Node::LockCallback callback = nullptr
    ) const {
        // Check if we can proceed with this lock intent
        if (!canProceedWithIntent(intent)) {
            return nullptr;
        }
        
        // Acquire a graph-level read lock to check if the node exists
        auto graphLock = lockGraph(LockIntent::Read, timeoutMs);
        if (!graphLock || !graphLock->isLocked()) {
            return nullptr;
        }
        
        auto it = nodes_.find(key);
        if (it == nodes_.end()) {
            return nullptr;
        }
        
        auto node = it->second;
        
        // Release graph lock before attempting node lock to prevent deadlocks
        graphLock->release();
        
        // Now try to lock the node
        return node->tryLock(intent, timeoutMs, callback);
    }

    /**
     * @brief Try to acquire an exclusive lock on the graph for structural changes
     * 
     * @param intent Purpose of the lock
     * @param timeoutMs Timeout in milliseconds (default: 100ms)
     * @return A graph lock handle or nullptr if acquisition failed
     */
    std::unique_ptr<GraphLockHandle> lockGraph(
        LockIntent intent,
        size_t timeoutMs = 100
    ) const {
        using namespace std::chrono;
        
        // For read locks, try to acquire immediately
        if (intent == LockIntent::Read) {
            std::shared_lock<std::shared_mutex> lock(graphMutex_, std::try_to_lock);
            if (lock.owns_lock()) {
                return std::make_unique<GraphLockHandle>(
                    const_cast<CoordinatedGraph*>(this), 
                    std::move(lock),
                    intent
                );
            }
            
            // If immediate acquisition failed, try with timeout
            auto start = steady_clock::now();
            while (true) {
                lock = std::shared_lock<std::shared_mutex>(graphMutex_, std::try_to_lock);
                if (lock.owns_lock()) {
                    return std::make_unique<GraphLockHandle>(
                        const_cast<CoordinatedGraph*>(this), 
                        std::move(lock),
                        intent
                    );
                }
                
                if (duration_cast<milliseconds>(steady_clock::now() - start).count() >= timeoutMs) {
                    return nullptr;
                }
                
                std::this_thread::sleep_for(milliseconds(1));
            }
        } 
        // For write/structure locks, need to notify existing holders
        else {
            // If this is a structure lock, notify all node lock holders
            if (intent == LockIntent::GraphStructure) {
                const_cast<CoordinatedGraph*>(this)->notifyAllNodeLockHolders(LockStatus::BackgroundWait);
            }
            
            // Try to acquire the write lock
            std::unique_lock<std::shared_mutex> lock(graphMutex_, std::try_to_lock);
            if (lock.owns_lock()) {
                // Record the current structural operation intent
                if (intent == LockIntent::GraphStructure) {
                    const_cast<CoordinatedGraph*>(this)->currentStructuralIntent_ = intent;
                }
                
                return std::make_unique<GraphLockHandle>(
                    const_cast<CoordinatedGraph*>(this), 
                    std::move(lock),
                    intent
                );
            }
            
            // If immediate acquisition failed, try with timeout
            auto start = steady_clock::now();
            while (true) {
                lock = std::unique_lock<std::shared_mutex>(graphMutex_, std::try_to_lock);
                if (lock.owns_lock()) {
                    // Record the current structural operation intent
                    if (intent == LockIntent::GraphStructure) {
                        const_cast<CoordinatedGraph*>(this)->currentStructuralIntent_ = intent;
                    }
                    
                    return std::make_unique<GraphLockHandle>(
                        const_cast<CoordinatedGraph*>(this), 
                        std::move(lock),
                        intent
                    );
                }
                
                if (duration_cast<milliseconds>(steady_clock::now() - start).count() >= timeoutMs) {
                    // Reset any notifications we sent
                    if (intent == LockIntent::GraphStructure) {
                        const_cast<CoordinatedGraph*>(this)->notifyAllNodeLockHolders(LockStatus::Acquired);
                    }
                    
                    return nullptr;
                }
                
                std::this_thread::sleep_for(milliseconds(1));
            }
        }
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
        auto lock = lockGraph(LockIntent::GraphStructure);
        if (!lock || !lock->isLocked()) {
            throw LockAcquisitionException("Failed to acquire graph lock for edge addition");
        }
        
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
        auto lock = lockGraph(LockIntent::GraphStructure);
        if (!lock || !lock->isLocked()) {
            throw LockAcquisitionException("Failed to acquire graph lock for edge removal");
        }
        
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
        auto lock = lockGraph(LockIntent::Read);
        if (!lock || !lock->isLocked()) {
            throw LockAcquisitionException("Failed to acquire graph lock for edge check");
        }
        
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
        auto lock = lockGraph(LockIntent::Read);
        if (!lock || !lock->isLocked()) {
            throw LockAcquisitionException("Failed to acquire graph lock for getting outgoing edges");
        }
        
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
        auto lock = lockGraph(LockIntent::Read);
        if (!lock || !lock->isLocked()) {
            throw LockAcquisitionException("Failed to acquire graph lock for getting incoming edges");
        }
        
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
        auto lock = lockGraph(LockIntent::Read);
        if (!lock || !lock->isLocked()) {
            throw LockAcquisitionException("Failed to acquire graph lock for cycle detection");
        }
        
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
        // Make a copy of the graph structure under a short-term lock to prevent
        // long-term lock holding during graph traversal
        std::unordered_map<KeyType, std::unordered_set<KeyType>> localOutEdges;
        std::unordered_set<KeyType> localNodes;
        
        auto lock = lockGraph(LockIntent::Read);
        if (!lock || !lock->isLocked()) {
            throw LockAcquisitionException("Failed to acquire graph lock for topological sort");
        }
        
        // If the graph is empty, return an empty result
        if (nodes_.empty()) {
            return {};
        }
        
        // Create local copies of the graph structure
        for (const auto& [key, _] : nodes_) {
            localNodes.insert(key);
            
            auto edgeIt = outEdges_.find(key);
            if (edgeIt != outEdges_.end()) {
                localOutEdges[key] = edgeIt->second;
            } else {
                localOutEdges[key] = {};
            }
        }
        
        // Release the lock before performing the sort
        lock->release();
        
        // Now perform the topological sort using our local copies
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
            
            // Use local graph structure
            auto edgeIt = localOutEdges.find(key);
            if (edgeIt != localOutEdges.end()) {
                for (const auto& neighbor : edgeIt->second) {
                    // Check if neighbor exists in local nodes
                    if (localNodes.find(neighbor) == localNodes.end()) {
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
        
        for (const auto& node : localNodes) {
            if (!visited[node]) {
                if (!visit(node)) {
                    return {};  // Cycle detected
                }
            }
        }
        
        std::reverse(result.begin(), result.end());
        return result;
    }

    /**
     * @brief Execute a function with automatic node locking
     * 
     * @param key Node identifier
     * @param func Function to execute with the node data
     * @param forWrite Whether to acquire a write lock
     * @param timeoutMs Timeout in milliseconds (default: 100ms)
     * @return true if the function was executed, false if the node doesn't exist or lock acquisition failed
     */
    template <typename Func>
    bool withNode(const KeyType& key, Func func, bool forWrite = false, size_t timeoutMs = 100) {
        auto intent = forWrite ? LockIntent::NodeModify : LockIntent::Read;
        auto nodeLock = tryLockNode(key, intent, forWrite, timeoutMs);
        
        if (!nodeLock || !nodeLock->isLocked()) {
            return false;
        }
        
        auto node = nodeLock->getNode();
        if (!node) {
            return false;
        }
        
        if (forWrite) {
            func(node->getData());
        } else {
            func(static_cast<const T&>(node->getData()));
        }
        
        return true;
    }

    /**
     * @brief Process nodes in dependency order, ensuring dependencies are processed before dependents
     * 
     * @param processFunc Function to call for each node
     * @return true if all nodes were processed, false if a cycle was detected
     */
    bool processDependencyOrder(std::function<void(const KeyType&, T&)> processFunc) {
        // First, compute a topological sort with a shared lock
        std::vector<KeyType> sortedNodes;
        {
            auto lock = lockGraph(LockIntent::Read);
            if (!lock || !lock->isLocked()) {
                throw LockAcquisitionException("Failed to acquire graph lock for dependency processing");
            }
            
            sortedNodes = topologicalSort();
            if (sortedNodes.empty() && !nodes_.empty()) {
                return false;  // Cycle detected
            }
        }
        
        // Process nodes in topological order - release graph lock between node operations
        // to prevent potential deadlocks
        for (const auto& key : sortedNodes) {
            auto nodeLock = tryLockNode(key, LockIntent::NodeModify, true, 100);
            if (!nodeLock || !nodeLock->isLocked()) {
                continue; // Skip if we couldn't lock the node
            }
            
            auto node = nodeLock->getNode();
            if (!node) {
                continue;
            }
            
            processFunc(key, node->getData());
        }
        
        return true;
    }

    /**
     * @brief Traverse the graph in breadth-first order starting from a node
     * 
     * @param startKey Key of the starting node
     * @param visitFunc Function to call for each visited node
     */
    void bfs(const KeyType& startKey, std::function<void(const KeyType&, const T&)> visitFunc) const {
        // Make local copies of the graph structure to minimize lock duration
        std::unordered_map<KeyType, std::unordered_set<KeyType>> localOutEdges;
        std::unordered_map<KeyType, std::shared_ptr<Node>> localNodes;
        
        // Get the starting node and its edges
        {
            auto lock = lockGraph(LockIntent::Read);
            if (!lock || !lock->isLocked()) {
                throw LockAcquisitionException("Failed to acquire graph lock for BFS");
            }
            
            auto nodeIt = nodes_.find(startKey);
            if (nodeIt == nodes_.end()) {
                return;  // Start node doesn't exist
            }
            
            // Copy the nodes and edges we need
            localNodes[startKey] = nodeIt->second;
            
            auto edgeIt = outEdges_.find(startKey);
            if (edgeIt != outEdges_.end()) {
                localOutEdges[startKey] = edgeIt->second;
            }
        }
        
        // Set up BFS
        std::queue<KeyType> queue;
        std::unordered_set<KeyType> visited;
        
        // Start with the initial node
        queue.push(startKey);
        visited.insert(startKey);
        
        // Process the first node
        {
            auto nodeLock = tryLockNode(startKey, LockIntent::Read, false, 50);
            if (nodeLock && nodeLock->isLocked()) {
                auto node = nodeLock->getNode();
                if (node) {
                    visitFunc(startKey, node->getData());
                }
            }
        }
        
        // BFS main loop
        while (!queue.empty()) {
            KeyType current = queue.front();
            queue.pop();
            
            // Get the neighbors if we don't already have them locally
            if (localOutEdges.find(current) == localOutEdges.end()) {
                auto lock = lockGraph(LockIntent::Read);
                if (!lock || !lock->isLocked()) {
                    continue;  // Skip if we can't get a lock
                }
                
                auto edgeIt = outEdges_.find(current);
                if (edgeIt != outEdges_.end()) {
                    localOutEdges[current] = edgeIt->second;
                } else {
                    localOutEdges[current] = {};
                }
            }
            
            // Process neighbors
            for (const auto& neighbor : localOutEdges[current]) {
                if (visited.find(neighbor) == visited.end()) {
                    visited.insert(neighbor);
                    queue.push(neighbor);
                    
                    // Get the node if we don't already have it locally
                    if (localNodes.find(neighbor) == localNodes.end()) {
                        auto lock = lockGraph(LockIntent::Read);
                        if (!lock || !lock->isLocked()) {
                            continue;  // Skip if we can't get a lock
                        }
                        
                        auto nodeIt = nodes_.find(neighbor);
                        if (nodeIt != nodes_.end()) {
                            localNodes[neighbor] = nodeIt->second;
                        }
                    }
                    
                    // Visit the neighbor
                    auto nodeLock = tryLockNode(neighbor, LockIntent::Read, false, 50);
                    if (nodeLock && nodeLock->isLocked()) {
                        auto node = nodeLock->getNode();
                        if (node) {
                            visitFunc(neighbor, node->getData());
                        }
                    }
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
    void dfs(const KeyType& startKey, std::function<void(const KeyType&, const T&)> visitFunc) const {
        // Make local copies of the graph structure to minimize lock duration
        std::unordered_map<KeyType, std::unordered_set<KeyType>> localOutEdges;
        
        // Get the starting node and its edges
        {
            auto lock = lockGraph(LockIntent::Read);
            if (!lock || !lock->isLocked()) {
                throw LockAcquisitionException("Failed to acquire graph lock for DFS");
            }
            
            auto nodeIt = nodes_.find(startKey);
            if (nodeIt == nodes_.end()) {
                return;  // Start node doesn't exist
            }
            
            auto edgeIt = outEdges_.find(startKey);
            if (edgeIt != outEdges_.end()) {
                localOutEdges[startKey] = edgeIt->second;
            }
        }
        
        // Set up DFS
        std::unordered_set<KeyType> visited;
        std::stack<KeyType> stack;
        
        // Start with the initial node
        stack.push(startKey);
        
        // DFS main loop
        while (!stack.empty()) {
            KeyType current = stack.top();
            stack.pop();
            
            if (visited.find(current) != visited.end()) {
                continue;  // Skip already visited nodes
            }
            
            visited.insert(current);
            
            // Visit the node
            auto nodeLock = tryLockNode(current, LockIntent::Read, false, 50);
            if (nodeLock && nodeLock->isLocked()) {
                auto node = nodeLock->getNode();
                if (node) {
                    visitFunc(current, node->getData());
                }
            }
            
            // Get the neighbors if we don't already have them locally
            if (localOutEdges.find(current) == localOutEdges.end()) {
                auto lock = lockGraph(LockIntent::Read);
                if (!lock || !lock->isLocked()) {
                    continue;  // Skip if we can't get a lock
                }
                
                auto edgeIt = outEdges_.find(current);
                if (edgeIt != outEdges_.end()) {
                    localOutEdges[current] = edgeIt->second;
                } else {
                    localOutEdges[current] = {};
                }
            }
            
            // Push neighbors onto the stack in reverse order to maintain DFS order
            std::vector<KeyType> neighbors(localOutEdges[current].begin(), localOutEdges[current].end());
            for (auto it = neighbors.rbegin(); it != neighbors.rend(); ++it) {
                if (visited.find(*it) == visited.end()) {
                    stack.push(*it);
                }
            }
        }
    }

    /**
     * @brief Get all node keys in the graph
     * 
     * @return Vector of all node keys
     */
    std::vector<KeyType> getAllNodes() const {
        auto lock = lockGraph(LockIntent::Read);
        if (!lock || !lock->isLocked()) {
            throw LockAcquisitionException("Failed to acquire graph lock for getting all nodes");
        }
        
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
        auto lock = lockGraph(LockIntent::Read);
        if (!lock || !lock->isLocked()) {
            throw LockAcquisitionException("Failed to acquire graph lock for getting size");
        }
        
        return nodes_.size();
    }

    /**
     * @brief Check if the graph is empty
     * 
     * @return true if the graph has no nodes, false otherwise
     */
    bool empty() const {
        auto lock = lockGraph(LockIntent::Read);
        if (!lock || !lock->isLocked()) {
            throw LockAcquisitionException("Failed to acquire graph lock for checking emptiness");
        }
        
        return nodes_.empty();
    }

    /**
     * @brief Clear all nodes and edges from the graph
     */
    void clear() {
        auto lock = lockGraph(LockIntent::GraphStructure);
        if (!lock || !lock->isLocked()) {
            throw LockAcquisitionException("Failed to acquire graph lock for clearing");
        }
        
        // Notify all nodes that they're being removed
        for (const auto& [_, node] : nodes_) {
            if (node) {
                node->notifyLockHolders(LockStatus::Preempted);
            }
        }
        
        nodes_.clear();
        outEdges_.clear();
        inEdges_.clear();
    }

    /**
     * @brief Register a callback for when a node is removed
     * 
     * @param callback Function to call when a node is removed
     * @return A string ID that can be used to unregister the callback
     */
    std::string registerNodeRemovalCallback(std::function<void(const KeyType&)> callback) {
        std::string id = std::to_string(++callbackCounter_);
        std::lock_guard<std::mutex> lock(callbackMutex_);
        removalCallbacks_[id] = std::move(callback);
        return id;
    }

    /**
     * @brief Unregister a node removal callback
     * 
     * @param id ID of the callback to remove
     * @return true if the callback was removed, false if not found
     */
    bool unregisterNodeRemovalCallback(const std::string& id) {
        std::lock_guard<std::mutex> lock(callbackMutex_);
        auto it = removalCallbacks_.find(id);
        if (it != removalCallbacks_.end()) {
            removalCallbacks_.erase(it);
            return true;
        }
        return false;
    }

private:
    friend class GraphLockHandle;
    friend class NodeLockHandle;
    
    // We've inlined the Node::tryLock method directly above
    
    /**
     * @brief Called when a graph lock is released
     * 
     * @param intent Intent of the lock that was released
     */
    void onGraphLockReleased(LockIntent intent) {
        if (intent == LockIntent::GraphStructure) {
            // Clear the current structural intent
            currentStructuralIntent_ = std::nullopt;
            
            // Notify all nodes that the structural operation is complete
            notifyAllNodeLockHolders(LockStatus::Acquired);
        }
    }
    
    /**
     * @brief Notify all node lock holders about a status change
     * 
     * @param status New status to notify
     */
    void notifyAllNodeLockHolders(LockStatus status) {
        std::lock_guard<std::shared_mutex> lock(graphMutex_);
        for (const auto& [_, node] : nodes_) {
            if (node) {
                node->notifyLockHolders(status);
            }
        }
    }
    
    /**
     * @brief Check if a lock intent can proceed given the current state
     * 
     * @param intent Intent to check
     * @return true if the intent can proceed, false otherwise
     */
    bool canProceedWithIntent(LockIntent intent) const {
        // If a graph structural operation is in progress and this is not a read intent,
        // we should wait
        if (currentStructuralIntent_ && 
            currentStructuralIntent_ == LockIntent::GraphStructure && 
            intent != LockIntent::Read) {
            return false;
        }
        
        return true;
    }
    
    /**
     * @brief Called when a node is removed from the graph
     * 
     * @param key Key of the removed node
     */
    void onNodeRemoved(const KeyType& key) {
        std::lock_guard<std::mutex> lock(callbackMutex_);
        for (const auto& [_, callback] : removalCallbacks_) {
            if (callback) {
                callback(key);
            }
        }
    }

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
    
    // Callbacks for node removal notification
    std::mutex callbackMutex_;
    std::unordered_map<std::string, std::function<void(const KeyType&)>> removalCallbacks_;
    std::atomic<size_t> callbackCounter_{0};
    
    // Track current structural operation intent to help with concurrency
    std::optional<LockIntent> currentStructuralIntent_ = std::nullopt;
};

} // namespace Fabric