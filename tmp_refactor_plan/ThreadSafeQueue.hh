#pragma once

#include <condition_variable>
#include <mutex>
#include <queue>
#include <optional>

namespace Fabric {

/**
 * @brief Thread-safe queue implementation
 * 
 * This class provides a thread-safe queue with operations that
 * handle synchronization automatically.
 * 
 * @tparam T Type of elements in the queue
 */
template <typename T>
class ThreadSafeQueue {
public:
    ThreadSafeQueue() = default;
    
    /**
     * @brief Add an item to the queue
     * 
     * @param item Item to add
     */
    void push(const T& item) {
        {
            std::lock_guard<std::timed_mutex> lock(mutex_);
            queue_.push(item);
        }
        condition_.notify_one();
    }
    
    /**
     * @brief Try to get an item from the queue without waiting
     * 
     * @param[out] item The item from the queue
     * @return true if an item was retrieved, false if the queue was empty
     */
    bool tryPop(T& item) {
        std::lock_guard<std::timed_mutex> lock(mutex_);
        if (queue_.empty()) {
            return false;
        }
        
        item = queue_.front();
        queue_.pop();
        return true;
    }
    
    /**
     * @brief Get an item from the queue, waiting if necessary
     * 
     * @param[out] item The item from the queue
     * @param predicate Additional condition to check
     * @return true if an item was retrieved, false if the wait was interrupted
     */
    template <typename Predicate>
    bool waitAndPop(T& item, Predicate predicate) {
        std::unique_lock<std::timed_mutex> lock(mutex_);
        condition_.wait(lock, [this, &predicate] { 
            return !queue_.empty() || predicate(); 
        });
        
        if (queue_.empty()) {
            return false;
        }
        
        item = queue_.front();
        queue_.pop();
        return true;
    }
    
    /**
     * @brief Get an item from the queue with a timeout
     * 
     * @param[out] item The item from the queue
     * @param timeoutMs Timeout in milliseconds
     * @param predicate Additional condition to check
     * @return true if an item was retrieved, false if the timeout expired
     */
    template <typename Predicate>
    bool waitAndPopWithTimeout(T& item, int timeoutMs, Predicate predicate) {
        std::unique_lock<std::timed_mutex> lock(mutex_);
        bool result = condition_.wait_for(lock, 
            std::chrono::milliseconds(timeoutMs),
            [this, &predicate] { 
                return !queue_.empty() || predicate(); 
            });
        
        if (!result || queue_.empty()) {
            return false;
        }
        
        item = queue_.front();
        queue_.pop();
        return true;
    }
    
    /**
     * @brief Check if the queue is empty
     * 
     * @return true if the queue is empty
     */
    bool empty() const {
        std::lock_guard<std::timed_mutex> lock(mutex_);
        return queue_.empty();
    }
    
    /**
     * @brief Get the size of the queue
     * 
     * @return Number of items in the queue
     */
    size_t size() const {
        std::lock_guard<std::timed_mutex> lock(mutex_);
        return queue_.size();
    }
    
    /**
     * @brief Clear all items from the queue
     */
    void clear() {
        std::lock_guard<std::timed_mutex> lock(mutex_);
        std::queue<T> empty;
        std::swap(queue_, empty);
    }

private:
    mutable std::timed_mutex mutex_;
    std::condition_variable_any condition_;
    std::queue<T> queue_;
};

} // namespace Fabric