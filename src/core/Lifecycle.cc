#include "fabric/core/Lifecycle.hh"
#include "fabric/utils/ErrorHandling.hh"
#include "fabric/utils/Logging.hh"
#include "fabric/utils/Utils.hh"

namespace Fabric {

std::string lifecycleStateToString(LifecycleState state) {
  switch (state) {
    case LifecycleState::Created:
      return "Created";
    case LifecycleState::Initialized:
      return "Initialized";
    case LifecycleState::Rendered:
      return "Rendered";
    case LifecycleState::Updating:
      return "Updating";
    case LifecycleState::Suspended:
      return "Suspended";
    case LifecycleState::Destroyed:
      return "Destroyed";
    default:
      return "Unknown";
  }
}

LifecycleManager::LifecycleManager() : currentState(LifecycleState::Created) {}

void LifecycleManager::setState(LifecycleState state) {
  std::vector<LifecycleHook> stateHooksToInvoke;
  std::vector<LifecycleHook> transitionHooksToInvoke;
  LifecycleState oldState;
  
  {
    std::lock_guard<std::mutex> lock(stateMutex);
    
    if (!isValidTransition(currentState, state)) {
      throwError("Invalid lifecycle state transition from " + 
                lifecycleStateToString(currentState) + " to " + 
                lifecycleStateToString(state));
    }
    
    oldState = currentState;
    currentState = state;
  }
  
  Logger::logDebug("Lifecycle state changed from " + 
                  lifecycleStateToString(oldState) + " to " + 
                  lifecycleStateToString(state));
  
  {
    std::lock_guard<std::mutex> lock(hooksMutex);
    
    // Collect state hooks to invoke
    auto it = stateHooks.find(state);
    if (it != stateHooks.end()) {
      for (const auto& entry : it->second) {
        stateHooksToInvoke.push_back(entry.hook);
      }
    }
    
    // Collect transition hooks to invoke
    std::string transitionKey = generateTransitionKey(oldState, state);
    auto transIt = transitionHooks.find(transitionKey);
    if (transIt != transitionHooks.end()) {
      for (const auto& entry : transIt->second) {
        transitionHooksToInvoke.push_back(entry.hook);
      }
    }
  }
  
  // Execute state hooks (outside the lock)
  for (const auto& hook : stateHooksToInvoke) {
    try {
      hook();
    } catch (const std::exception& e) {
      Logger::logError("Exception in lifecycle hook: " + std::string(e.what()));
    } catch (...) {
      Logger::logError("Unknown exception in lifecycle hook");
    }
  }
  
  // Execute transition hooks (outside the lock)
  for (const auto& hook : transitionHooksToInvoke) {
    try {
      hook();
    } catch (const std::exception& e) {
      Logger::logError("Exception in lifecycle transition hook: " + std::string(e.what()));
    } catch (...) {
      Logger::logError("Unknown exception in lifecycle transition hook");
    }
  }
}

LifecycleState LifecycleManager::getState() const {
  std::lock_guard<std::mutex> lock(stateMutex);
  return currentState;
}

std::string LifecycleManager::addHook(LifecycleState state, const LifecycleHook& hook) {
  if (!hook) {
    throwError("Lifecycle hook cannot be null");
  }
  
  std::lock_guard<std::mutex> lock(hooksMutex);
  
  HookEntry entry;
  entry.id = Utils::generateUniqueId("hook_");
  entry.hook = hook;
  
  stateHooks[state].push_back(entry);
  Logger::logDebug("Added lifecycle hook for state '" + 
                  lifecycleStateToString(state) + "' with ID '" + entry.id + "'");
  
  return entry.id;
}

std::string LifecycleManager::addTransitionHook(LifecycleState fromState, LifecycleState toState, 
                                            const LifecycleHook& hook) {
  if (!hook) {
    throwError("Lifecycle transition hook cannot be null");
  }
  
  std::lock_guard<std::mutex> lock(hooksMutex);
  
  HookEntry entry;
  entry.id = Utils::generateUniqueId("transition_");
  entry.hook = hook;
  
  std::string transitionKey = generateTransitionKey(fromState, toState);
  transitionHooks[transitionKey].push_back(entry);
  Logger::logDebug("Added lifecycle transition hook from '" + 
                  lifecycleStateToString(fromState) + "' to '" + 
                  lifecycleStateToString(toState) + "' with ID '" + entry.id + "'");
  
  return entry.id;
}

bool LifecycleManager::removeHook(const std::string& hookId) {
  std::lock_guard<std::mutex> lock(hooksMutex);
  
  // Check state hooks
  for (auto& [state, hooks] : stateHooks) {
    auto it = std::find_if(hooks.begin(), hooks.end(),
                          [&hookId](const HookEntry& entry) { return entry.id == hookId; });
    
    if (it != hooks.end()) {
      hooks.erase(it);
      Logger::logDebug("Removed lifecycle hook with ID '" + hookId + "'");
      return true;
    }
  }
  
  // Check transition hooks
  for (auto& [transition, hooks] : transitionHooks) {
    auto it = std::find_if(hooks.begin(), hooks.end(),
                          [&hookId](const HookEntry& entry) { return entry.id == hookId; });
    
    if (it != hooks.end()) {
      hooks.erase(it);
      Logger::logDebug("Removed lifecycle transition hook with ID '" + hookId + "'");
      return true;
    }
  }
  
  return false;
}

bool LifecycleManager::isValidTransition(LifecycleState fromState, LifecycleState toState) {
  // Self-transitions are always valid
  if (fromState == toState) {
    return true;
  }
  
  switch (fromState) {
    case LifecycleState::Created:
      // Created -> Initialized or Destroyed
      return toState == LifecycleState::Initialized ||
            toState == LifecycleState::Destroyed;
      
    case LifecycleState::Initialized:
      // Initialized -> Rendered, Suspended, or Destroyed
      return toState == LifecycleState::Rendered ||
            toState == LifecycleState::Suspended ||
            toState == LifecycleState::Destroyed;
      
    case LifecycleState::Rendered:
      // Rendered -> Updating, Suspended, or Destroyed
      return toState == LifecycleState::Updating ||
            toState == LifecycleState::Suspended ||
            toState == LifecycleState::Destroyed;
      
    case LifecycleState::Updating:
      // Updating -> Rendered, Suspended, or Destroyed
      return toState == LifecycleState::Rendered ||
            toState == LifecycleState::Suspended ||
            toState == LifecycleState::Destroyed;
      
    case LifecycleState::Suspended:
      // Suspended -> Initialized, Rendered, or Destroyed
      return toState == LifecycleState::Initialized ||
            toState == LifecycleState::Rendered ||
            toState == LifecycleState::Destroyed;
      
    case LifecycleState::Destroyed:
      // Destroyed is a terminal state
      return false;
      
    default:
      return false;
  }
}

std::string LifecycleManager::generateTransitionKey(LifecycleState fromState, LifecycleState toState) {
  return std::to_string(static_cast<int>(fromState)) + ":" + std::to_string(static_cast<int>(toState));
}

} // namespace Fabric