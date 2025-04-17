#include "fabric/utils/ThreadPoolExecutor.hh"
#include "fabric/utils/Logging.hh"
#include <algorithm>
#include <iostream>

namespace Fabric {
namespace Utils {

ThreadPoolExecutor::ThreadPoolExecutor(size_t threadCount)
    : threadCount_(threadCount > 0 ? threadCount : std::thread::hardware_concurrency()) {
    // Start the worker threads
    workerThreads_.reserve(threadCount_);
    for (size_t i = 0; i < threadCount_; ++i) {
        workerThreads_.emplace_back(&ThreadPoolExecutor::workerThread, this);
    }
    
    Logger::logDebug("ThreadPoolExecutor created with " + std::to_string(threadCount_) + " threads");
}

ThreadPoolExecutor::~ThreadPoolExecutor() {
    // Shutdown the thread pool if not already done
    if (!shutdown_) {
        try {
            // Use a shorter timeout in the destructor to avoid blocking
            shutdown(std::chrono::milliseconds(200));
        } catch (const std::exception& e) {
            // Log the error but continue destruction
            Logger::logError("Error during ThreadPoolExecutor shutdown: " + std::string(e.what()));
        } catch (...) {
            Logger::logError("Unknown error during ThreadPoolExecutor shutdown");
        }
    }
}

ThreadPoolExecutor::ThreadPoolExecutor(ThreadPoolExecutor&& other) noexcept
    : threadCount_(other.threadCount_),
      shutdown_(other.shutdown_.load()),
      pausedForTesting_(other.pausedForTesting_.load()) {
    
    // Move the task queue
    {
        std::lock_guard<std::mutex> lock(other.queueMutex_);
        taskQueue_ = std::move(other.taskQueue_);
    }
    
    // Move the threads
    workerThreads_ = std::move(other.workerThreads_);
    
    // Reset the other thread pool
    other.threadCount_ = 0;
    other.shutdown_ = true;
}

ThreadPoolExecutor& ThreadPoolExecutor::operator=(ThreadPoolExecutor&& other) noexcept {
    if (this != &other) {
        // Shutdown this thread pool
        if (!shutdown_) {
            try {
                shutdown(std::chrono::milliseconds(100));
            } catch (...) {
                // Ignore exceptions in move assignment
            }
        }
        
        // Move from the other thread pool
        threadCount_ = other.threadCount_;
        shutdown_ = other.shutdown_.load();
        pausedForTesting_ = other.pausedForTesting_.load();
        
        // Move the task queue
        {
            std::lock_guard<std::mutex> lock(other.queueMutex_);
            taskQueue_ = std::move(other.taskQueue_);
        }
        
        // Move the threads
        workerThreads_ = std::move(other.workerThreads_);
        
        // Reset the other thread pool
        other.threadCount_ = 0;
        other.shutdown_ = true;
    }
    
    return *this;
}

void ThreadPoolExecutor::setThreadCount(size_t count) {
    if (count == 0) {
        throw std::invalid_argument("Thread count must be at least 1");
    }
    
    // Store the current thread count
    size_t oldCount = threadCount_;
    
    // Set the new thread count
    threadCount_ = count;
    
    // If we're reducing the thread count
    if (count < oldCount) {
        std::lock_guard<std::mutex> lock(queueMutex_);
        
        // Notify worker threads that they should check their status
        queueCondition_.notify_all();
        
        // The excess threads will exit naturally in workerThread()
        // when they recheck threadCount_
    }
    
    // If we're increasing the thread count
    if (count > oldCount && !shutdown_ && !pausedForTesting_) {
        // Start new worker threads
        for (size_t i = oldCount; i < count; ++i) {
            workerThreads_.emplace_back(&ThreadPoolExecutor::workerThread, this);
        }
    }
    
    Logger::logDebug("ThreadPoolExecutor thread count changed from " + 
                    std::to_string(oldCount) + " to " + std::to_string(count));
}

size_t ThreadPoolExecutor::getThreadCount() const {
    return threadCount_;
}

bool ThreadPoolExecutor::shutdown(std::chrono::milliseconds timeout) {
    // Set the shutdown flag
    shutdown_ = true;
    
    // Notify all worker threads
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        queueCondition_.notify_all();
    }
    
    // Join all worker threads with timeout
    auto startTime = std::chrono::steady_clock::now();
    bool allJoined = true;
    
    for (auto& thread : workerThreads_) {
        if (thread.joinable()) {
            // Calculate remaining timeout
            auto elapsed = std::chrono::steady_clock::now() - startTime;
            auto remainingTime = timeout - std::chrono::duration_cast<std::chrono::milliseconds>(elapsed);
            
            if (remainingTime <= std::chrono::milliseconds::zero()) {
                // Timeout reached, don't wait for remaining threads
                allJoined = false;
                break;
            }
            
            // Create a future to join the thread with timeout
            bool threadJoined = false;
            auto joinFuture = std::async(std::launch::async, [&thread, &threadJoined]() {
                thread.join();
                threadJoined = true;
            });
            
            // Wait for the join future with timeout
            if (joinFuture.wait_for(remainingTime) == std::future_status::timeout) {
                // Thread join timed out
                allJoined = false;
                // Log a warning about the stuck thread
                Logger::logWarning("Thread join timed out during ThreadPoolExecutor shutdown");
                // Continue with other threads
                continue;
            }
        }
    }
    
    // Clear the worker threads vector
    workerThreads_.clear();
    
    // Clear the task queue
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        std::queue<Task> emptyQueue;
        std::swap(taskQueue_, emptyQueue);
    }
    
    if (!allJoined) {
        Logger::logWarning("ThreadPoolExecutor shutdown timed out, some threads may not have joined");
    } else {
        Logger::logDebug("ThreadPoolExecutor shut down successfully");
    }
    
    return allJoined;
}

bool ThreadPoolExecutor::isShutdown() const {
    return shutdown_;
}

void ThreadPoolExecutor::pauseForTesting() {
    // If we're already paused, do nothing
    if (pausedForTesting_) {
        return;
    }
    
    // Store the current thread count
    std::lock_guard<std::mutex> lock(queueMutex_);
    pausedForTesting_ = true;
    
    // Process any queued tasks immediately
    while (!taskQueue_.empty()) {
        Task task = std::move(taskQueue_.front());
        taskQueue_.pop();
        
        // Unlock the mutex before running the task
        // to prevent deadlocks
        queueMutex_.unlock();
        try {
            task();
        } catch (const std::exception& e) {
            Logger::logError("Exception in task during pauseForTesting: " + std::string(e.what()));
        } catch (...) {
            Logger::logError("Unknown exception in task during pauseForTesting");
        }
        queueMutex_.lock();
    }
    
    Logger::logDebug("ThreadPoolExecutor paused for testing");
}

void ThreadPoolExecutor::resumeAfterTesting() {
    // If we're not paused, do nothing
    if (!pausedForTesting_) {
        return;
    }
    
    // Resume normal operation
    pausedForTesting_ = false;
    
    // Restart worker threads if needed
    if (!shutdown_ && workerThreads_.size() < threadCount_) {
        size_t threadsToStart = threadCount_ - workerThreads_.size();
        for (size_t i = 0; i < threadsToStart; ++i) {
            workerThreads_.emplace_back(&ThreadPoolExecutor::workerThread, this);
        }
    }
    
    Logger::logDebug("ThreadPoolExecutor resumed after testing");
}

bool ThreadPoolExecutor::isPausedForTesting() const {
    return pausedForTesting_;
}

size_t ThreadPoolExecutor::getQueuedTaskCount() const {
    std::lock_guard<std::mutex> lock(queueMutex_);
    return taskQueue_.size();
}

void ThreadPoolExecutor::workerThread() {
    // Loop until shutdown or thread count reduced
    while (!shutdown_) {
        // Calculate this thread's index
        size_t threadIndex = 0;
        for (size_t i = 0; i < workerThreads_.size(); ++i) {
            if (workerThreads_[i].get_id() == std::this_thread::get_id()) {
                threadIndex = i;
                break;
            }
        }
        
        // Check if this thread should exit due to thread count reduction
        if (threadIndex >= threadCount_) {
            break;
        }
        
        // Get a task from the queue
        Task task;
        bool hasTask = false;
        {
            std::unique_lock<std::mutex> lock(queueMutex_);
            
            // Wait for a task or shutdown signal
            queueCondition_.wait(lock, [this, threadIndex] {
                return !taskQueue_.empty() || shutdown_ || pausedForTesting_ || threadIndex >= threadCount_;
            });
            
            // Check for shutdown or thread count reduction
            if (shutdown_ || pausedForTesting_ || threadIndex >= threadCount_) {
                break;
            }
            
            // Get the task
            if (!taskQueue_.empty()) {
                task = std::move(taskQueue_.front());
                taskQueue_.pop();
                hasTask = true;
            }
        }
        
        // Execute the task
        if (hasTask) {
            try {
                task();
            } catch (const std::exception& e) {
                // Log but don't terminate the worker thread
                Logger::logError("Exception in worker thread task: " + std::string(e.what()));
            } catch (...) {
                Logger::logError("Unknown exception in worker thread task");
            }
        }
    }
}

} // namespace Utils
} // namespace Fabric