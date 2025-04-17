#pragma once

#include <shared_mutex>
#include <functional>
#include <type_traits>
#include <optional>
#include "fabric/utils/TimeoutLock.hh"
#include "fabric/utils/Logging.hh"

namespace Fabric {
namespace Utils {

/**
 * @brief A trait-like interface for objects with lifecycle states
 * 
 * This template class provides state management functionality for objects
 * that have a well-defined lifecycle with state transitions. It manages 
 * thread-safe state transitions and provides hooks for derived classes
 * to define state-specific behavior.
 * 
 * @tparam State An enum or enum class defining the possible states
 * @tparam Derived The derived class that will implement the hooks
 */
template<typename State, typename Derived>
class LifecycleState {
public:
    /**
     * @brief Construct a new LifecycleState with the given initial state
     * 
     * @param initialState The initial state
     */
    explicit LifecycleState(State initialState)
        : state_(initialState) {}
        
    /**
     * @brief Virtual destructor
     */
    virtual ~LifecycleState() = default;
    
    /**
     * @brief Get the current state
     * 
     * @return The current state
     */
    State getState() const {
        auto lock = TimeoutLock<std::shared_mutex>::tryLockShared(stateMutex_);
        return lock ? state_ : State{}; // Return default state if lock fails
    }
    
    /**
     * @brief Try to transition to a new state
     * 
     * This method attempts to transition to the specified state by:
     * 1. Checking if the transition is valid
     * 2. Calling onExitState for the current state
     * 3. Updating the state
     * 4. Calling onEnterState for the new state
     * 
     * If any step fails, the state remains unchanged.
     * 
     * @param newState The state to transition to
     * @return true if the transition was successful, false otherwise
     */
    bool transitionTo(State newState) {
        // Get a write lock on the state
        auto lock = TimeoutLock<std::shared_mutex>::tryLockUnique(stateMutex_);
        if (!lock) {
            Logger::logWarning("Failed to acquire lock for state transition");
            return false;
        }
        
        // Check if the transition is valid
        if (!isValidTransition(state_, newState)) {
            Logger::logWarning("Invalid state transition from " + 
                              std::to_string(static_cast<int>(state_)) + 
                              " to " + std::to_string(static_cast<int>(newState)));
            return false;
        }
        
        // Store the current state
        State oldState = state_;
        
        try {
            // Call the exit hook for the current state
            derived()->onExitState(oldState);
            
            // Update the state
            state_ = newState;
            
            // Call the enter hook for the new state
            if (!derived()->onEnterState(newState)) {
                // If the enter hook fails, rollback to the old state
                state_ = oldState;
                
                // Try to call the enter hook for the old state to restore it
                try {
                    derived()->onEnterState(oldState);
                } catch (...) {
                    // If entering the old state fails, just log the error
                    Logger::logError("Failed to restore previous state " + 
                                    std::to_string(static_cast<int>(oldState)));
                }
                
                return false;
            }
            
            return true;
        } catch (const std::exception& e) {
            // Handle any exceptions by rolling back to the old state
            Logger::logError("Exception during state transition: " + std::string(e.what()));
            state_ = oldState;
            return false;
        }
    }
    
    /**
     * @brief Check if a transition from one state to another is valid
     * 
     * By default, all transitions are allowed. Derived classes can override
     * this method to implement specific state transition rules.
     * 
     * @param from The current state
     * @param to The target state
     * @return true if the transition is valid, false otherwise
     */
    virtual bool isValidTransition(State from, State to) const {
        // By default, allow all transitions
        return true;
    }
    
    /**
     * @brief Execute a function if the current state matches the specified state
     * 
     * This method is useful for conditional execution based on the current state.
     * 
     * @tparam Func Function type
     * @param state The state to check for
     * @param func The function to execute if the state matches
     * @return Result of the function if executed, std::nullopt otherwise
     */
    template<typename Func>
    auto ifInState(State state, Func&& func) 
    -> std::optional<std::invoke_result_t<Func>> {
        using ResultType = std::invoke_result_t<Func>;
        
        // Get a read lock on the state
        auto lock = TimeoutLock<std::shared_mutex>::tryLockShared(stateMutex_);
        if (!lock) {
            return std::nullopt;
        }
        
        // Check if the state matches
        if (state_ != state) {
            return std::nullopt;
        }
        
        // Execute the function
        if constexpr (std::is_same_v<ResultType, void>) {
            func();
            return std::optional<ResultType>(std::in_place);
        } else {
            return std::make_optional(func());
        }
    }
    
    /**
     * @brief Execute a function with the current state
     * 
     * This method is useful for operations that need to know the current state.
     * 
     * @tparam Func Function type
     * @param func The function to execute with the current state
     * @return Result of the function if executed, std::nullopt otherwise
     */
    template<typename Func>
    auto withState(Func&& func) 
    -> std::optional<std::invoke_result_t<Func, State>> {
        using ResultType = std::invoke_result_t<Func, State>;
        
        // Get a read lock on the state
        auto lock = TimeoutLock<std::shared_mutex>::tryLockShared(stateMutex_);
        if (!lock) {
            return std::nullopt;
        }
        
        // Execute the function with the current state
        if constexpr (std::is_same_v<ResultType, void>) {
            func(state_);
            return std::optional<ResultType>(std::in_place);
        } else {
            return std::make_optional(func(state_));
        }
    }
    
protected:
    /**
     * @brief Hook called when entering a new state
     * 
     * Derived classes must implement this method to define behavior
     * when entering a specific state.
     * 
     * @param state The state being entered
     * @return true if entering the state was successful, false otherwise
     */
    virtual bool onEnterState(State state) = 0;
    
    /**
     * @brief Hook called when exiting a state
     * 
     * Derived classes must implement this method to define behavior
     * when exiting a specific state.
     * 
     * @param state The state being exited
     */
    virtual void onExitState(State state) = 0;
    
private:
    /**
     * @brief Get a pointer to the derived class
     * 
     * This method uses the Curiously Recurring Template Pattern (CRTP)
     * to access the derived class.
     * 
     * @return Pointer to the derived class
     */
    Derived* derived() {
        return static_cast<Derived*>(this);
    }
    
    State state_;
    mutable std::shared_mutex stateMutex_;
};

} // namespace Utils
} // namespace Fabric