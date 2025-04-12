#pragma once

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <mutex>

namespace Fabric {

/**
 * @brief Defines the lifecycle states of a component.
 */
enum class LifecycleState {
  Created,     // Component has been created but not initialized
  Initialized, // Component has been initialized
  Rendered,    // Component has been rendered at least once
  Updating,    // Component is currently in the update loop
  Suspended,   // Component is temporarily inactive
  Destroyed    // Component has been destroyed
};

/**
 * @brief Hook function type for lifecycle events
 */
using LifecycleHook = std::function<void()>;

/**
 * @brief Manages the lifecycle of a component
 * 
 * The LifecycleManager tracks the current state of a component and
 * allows registering hooks for lifecycle state changes.
 */
class LifecycleManager {
public:
  /**
   * @brief Default constructor
   */
  LifecycleManager();

  /**
   * @brief Set the current lifecycle state
   * 
   * @param state New lifecycle state
   * @throws FabricException if the transition is invalid
   */
  void setState(LifecycleState state);

  /**
   * @brief Get the current lifecycle state
   * 
   * @return Current lifecycle state
   */
  LifecycleState getState() const;

  /**
   * @brief Register a hook to be called when transitioning to a specific state
   * 
   * @param state State to hook
   * @param hook Function to call
   * @return Hook ID for removal
   * @throws FabricException if hook is null
   */
  std::string addHook(LifecycleState state, const LifecycleHook& hook);

  /**
   * @brief Register a hook to be called when transitioning from one state to another
   * 
   * @param fromState State to transition from
   * @param toState State to transition to
   * @param hook Function to call
   * @return Hook ID for removal
   * @throws FabricException if hook is null
   */
  std::string addTransitionHook(LifecycleState fromState, LifecycleState toState, 
                              const LifecycleHook& hook);

  /**
   * @brief Remove a hook by ID
   * 
   * @param hookId Hook ID to remove
   * @return true if the hook was removed, false otherwise
   */
  bool removeHook(const std::string& hookId);

  /**
   * @brief Check if a transition is valid
   * 
   * @param fromState State to transition from
   * @param toState State to transition to
   * @return true if the transition is valid, false otherwise
   */
  static bool isValidTransition(LifecycleState fromState, LifecycleState toState);

private:
  mutable std::mutex stateMutex;
  LifecycleState currentState;
  
  struct HookEntry {
    std::string id;
    LifecycleHook hook;
  };
  
  mutable std::mutex hooksMutex;
  std::unordered_map<LifecycleState, std::vector<HookEntry>> stateHooks;
  std::unordered_map<std::string, std::vector<HookEntry>> transitionHooks;
  
  /**
   * @brief Generate a string key for transition hooks
   * 
   * @param fromState State to transition from
   * @param toState State to transition to
   * @return Key string in format "from:to"
   */
  static std::string generateTransitionKey(LifecycleState fromState, LifecycleState toState);
};

/**
 * @brief Convert a LifecycleState to a string
 * 
 * @param state Lifecycle state
 * @return String representation of the state
 */
std::string lifecycleStateToString(LifecycleState state);

} // namespace Fabric