#pragma once

#include "fabric/core/Resource.hh"
#include "fabric/utils/concurrency/ThreadSafeQueue.hh"
#include <atomic>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

namespace Fabric {

/**
 * @brief Thread pool for asynchronous resource loading
 * 
 * This class manages worker threads that process resource loading
 * requests in the background.
 */
class ResourceThreadPool {
public:
    /**
     * @brief Constructor
     * 
     * @param threadCount Number of worker threads
     */
    explicit ResourceThreadPool(unsigned int threadCount = std::thread::hardware_concurrency());
    
    /**
     * @brief Destructor
     */
    ~ResourceThreadPool();
    
    /**
     * @brief Queue a resource loading request
     * 
     * @param request Resource loading request
     */
    void queueRequest(const ResourceLoadRequest &request);
    
    /**
     * @brief Get the worker thread count
     * 
     * @return Number of worker threads
     */
    unsigned int getWorkerThreadCount() const;
    
    /**
     * @brief Set the worker thread count
     * 
     * @param count Number of worker threads
     */
    void setWorkerThreadCount(unsigned int count);
    
    /**
     * @brief Disable worker threads for testing
     */
    void disableWorkerThreadsForTesting();
    
    /**
     * @brief Restart worker threads after testing
     */
    void restartWorkerThreadsAfterTesting();
    
    /**
     * @brief Shutdown the thread pool
     */
    void shutdown();
    
private:
    // Worker thread function
    void workerThreadFunc();
    
    // Process a resource loading request
    void processRequest(const ResourceLoadRequest &request);
    
    // Thread management
    std::atomic<unsigned int> workerThreadCount_;
    std::vector<std::unique_ptr<std::thread>> workerThreads_;
    std::atomic<bool> shutdown_{false};
    
    // Thread synchronization
    std::timed_mutex threadControlMutex_;
    
    // Request queue
    ThreadSafeQueue<ResourceLoadRequest> requestQueue_;
};

} // namespace Fabric