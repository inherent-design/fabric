#pragma once

#include <chrono>
#include <future>
#include <optional>
#include <string>
#include <type_traits>
#include "fabric/utils/Logging.hh"

namespace Fabric {
namespace Utils {

/**
 * @brief Exception thrown when an async operation times out
 */
class AsyncTimeoutException : public std::runtime_error {
public:
    explicit AsyncTimeoutException(const std::string& message)
        : std::runtime_error(message) {}
};

/**
 * @brief Utility for timeout-protected async operations
 * 
 * This class provides static methods for running functions asynchronously
 * with timeout protection and error handling.
 */
class AsyncRunner {
public:
    /**
     * @brief Run a function asynchronously with timeout
     * 
     * @tparam Func Function type
     * @tparam Args Argument types
     * @param timeout Maximum time to wait for the function to complete
     * @param func Function to execute
     * @param args Arguments to pass to the function
     * @return Result of the function if completed within timeout, std::nullopt otherwise
     */
    template<typename Func, typename... Args>
    static auto runWithTimeout(
        std::chrono::milliseconds timeout,
        Func&& func, 
        Args&&... args
    ) -> std::optional<std::invoke_result_t<Func, Args...>> {
        using ResultType = std::invoke_result_t<Func, Args...>;
        
        // Create a packaged task
        std::packaged_task<ResultType()> task(
            [f = std::forward<Func>(func), ... args = std::forward<Args>(args)]() mutable {
                return f(std::forward<Args>(args)...);
            }
        );
        
        // Get the future from the task
        std::future<ResultType> future = task.get_future();
        
        // Run the task in a separate thread
        std::thread thread(std::move(task));
        
        // Wait for the future with timeout
        auto status = future.wait_for(timeout);
        
        // Check if the task completed within the timeout
        if (status == std::future_status::timeout) {
            // Detach the thread to avoid blocking
            thread.detach();
            return std::nullopt;
        } else {
            // Join the thread
            thread.join();
            
            // Get the result
            if constexpr (std::is_same_v<ResultType, void>) {
                future.get();
                return std::optional<ResultType>(std::in_place);
            } else {
                return std::make_optional(future.get());
            }
        }
    }
    
    /**
     * @brief Run a function with error handling
     * 
     * @tparam Func Function type
     * @tparam Args Argument types
     * @param operationName Name of the operation for logging
     * @param func Function to execute
     * @param args Arguments to pass to the function
     * @return Result of the function if successful, std::nullopt otherwise
     */
    template<typename Func, typename... Args>
    static auto runWithErrorHandling(
        const std::string& operationName,
        Func&& func,
        Args&&... args
    ) -> std::optional<std::invoke_result_t<Func, Args...>> {
        using ResultType = std::invoke_result_t<Func, Args...>;
        
        try {
            // Execute the function
            if constexpr (std::is_same_v<ResultType, void>) {
                func(std::forward<Args>(args)...);
                return std::optional<ResultType>(std::in_place);
            } else {
                return std::make_optional(func(std::forward<Args>(args)...));
            }
        } catch (const std::exception& e) {
            // Log the error
            Logger::logError("Exception in " + operationName + ": " + std::string(e.what()));
            return std::nullopt;
        } catch (...) {
            // Log the error
            Logger::logError("Unknown exception in " + operationName);
            return std::nullopt;
        }
    }
    
    /**
     * @brief Run a function with both timeout and error handling
     * 
     * @tparam Func Function type
     * @tparam Args Argument types
     * @param operationName Name of the operation for logging
     * @param timeout Maximum time to wait for the function to complete
     * @param func Function to execute
     * @param args Arguments to pass to the function
     * @return Result of the function if successful and completed within timeout, std::nullopt otherwise
     */
    template<typename Func, typename... Args>
    static auto runWithTimeoutAndErrorHandling(
        const std::string& operationName,
        std::chrono::milliseconds timeout,
        Func&& func,
        Args&&... args
    ) -> std::optional<std::invoke_result_t<Func, Args...>> {
        using ResultType = std::invoke_result_t<Func, Args...>;
        
        // Create a packaged task that handles its own exceptions
        std::packaged_task<std::optional<ResultType>()> task(
            [operationName, f = std::forward<Func>(func), ... args = std::forward<Args>(args)]() mutable {
                try {
                    if constexpr (std::is_same_v<ResultType, void>) {
                        f(std::forward<Args>(args)...);
                        return std::optional<ResultType>(std::in_place);
                    } else {
                        return std::make_optional(f(std::forward<Args>(args)...));
                    }
                } catch (const std::exception& e) {
                    // Log the error
                    Logger::logError("Exception in " + operationName + ": " + std::string(e.what()));
                    return std::nullopt;
                } catch (...) {
                    // Log the error
                    Logger::logError("Unknown exception in " + operationName);
                    return std::nullopt;
                }
            }
        );
        
        // Get the future from the task
        std::future<std::optional<ResultType>> future = task.get_future();
        
        // Run the task in a separate thread
        std::thread thread(std::move(task));
        
        // Wait for the future with timeout
        auto status = future.wait_for(timeout);
        
        // Check if the task completed within the timeout
        if (status == std::future_status::timeout) {
            // Detach the thread to avoid blocking
            thread.detach();
            
            // Log the timeout
            Logger::logWarning(operationName + " timed out after " + 
                             std::to_string(timeout.count()) + "ms");
            
            return std::nullopt;
        } else {
            // Join the thread
            thread.join();
            
            // Return the result
            return future.get();
        }
    }
    
    /**
     * @brief Run multiple functions in parallel with timeout
     * 
     * @tparam Funcs Function types
     * @param timeout Maximum time to wait for all functions to complete
     * @param funcs Functions to execute
     * @return Vector of results, with std::nullopt for functions that failed or timed out
     */
    template<typename... Funcs>
    static auto runAllWithTimeout(
        std::chrono::milliseconds timeout,
        Funcs&&... funcs
    ) -> std::tuple<std::optional<std::invoke_result_t<Funcs>>...> {
        // Create a tuple to hold the results
        std::tuple<std::optional<std::invoke_result_t<Funcs>>...> results;
        
        // Start timer
        auto startTime = std::chrono::steady_clock::now();
        
        // Create tasks for each function
        std::tuple<std::future<std::invoke_result_t<Funcs>>...> futures;
        
        // Launch each function in a separate thread
        if constexpr (sizeof...(Funcs) > 0) {
            futures = std::make_tuple(
                std::async(std::launch::async, std::forward<Funcs>(funcs))...
            );
        }
        
        // Wait for all futures with timeout
        waitForFutures(startTime, timeout, results, futures, std::index_sequence_for<Funcs...>{});
        
        return results;
    }
    
private:
    // Helper function to wait for a tuple of futures with timeout
    template<typename ResultTuple, typename FutureTuple, std::size_t... I>
    static void waitForFutures(
        std::chrono::steady_clock::time_point startTime,
        std::chrono::milliseconds timeout,
        ResultTuple& results,
        FutureTuple& futures,
        std::index_sequence<I...>
    ) {
        // Wait for each future
        (waitForFuture(
            startTime, 
            timeout, 
            std::get<I>(results), 
            std::get<I>(futures)
        ), ...);
    }
    
    // Helper function to wait for a single future with timeout
    template<typename T>
    static void waitForFuture(
        std::chrono::steady_clock::time_point startTime,
        std::chrono::milliseconds timeout,
        std::optional<T>& result,
        std::future<T>& future
    ) {
        // Calculate remaining timeout
        auto elapsed = std::chrono::steady_clock::now() - startTime;
        auto remainingTimeout = timeout - std::chrono::duration_cast<std::chrono::milliseconds>(elapsed);
        
        if (remainingTimeout <= std::chrono::milliseconds::zero()) {
            // No time left, set result to nullopt
            result = std::nullopt;
            return;
        }
        
        // Wait for the future with remaining timeout
        auto status = future.wait_for(remainingTimeout);
        
        if (status == std::future_status::timeout) {
            // Future timed out, set result to nullopt
            result = std::nullopt;
        } else {
            try {
                // Get the result
                if constexpr (std::is_same_v<T, void>) {
                    future.get();
                    result = std::optional<T>(std::in_place);
                } else {
                    result = std::make_optional(future.get());
                }
            } catch (...) {
                // Future threw an exception, set result to nullopt
                result = std::nullopt;
            }
        }
    }
};

} // namespace Utils
} // namespace Fabric