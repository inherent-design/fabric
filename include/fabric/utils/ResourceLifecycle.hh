#pragma once

#include <atomic>
#include "fabric/utils/LifecycleState.hh"
#include "fabric/utils/Logging.hh"

namespace Fabric {
namespace Utils {

/**
 * @brief Common resource states
 */
enum class ResourceState {
    Unloaded,      // Resource is not loaded
    Loading,       // Resource is currently being loaded
    Loaded,        // Resource is fully loaded and ready to use
    LoadingFailed, // Resource failed to load
    Unloading      // Resource is being unloaded
};

/**
 * @brief A component for resource lifecycle management
 * 
 * This template class provides resource lifecycle management with load count tracking
 * and standardized load/unload operations. It builds on the LifecycleState pattern
 * to provide resource-specific state transitions.
 * 
 * @tparam Resource The resource type that will implement the required hooks
 */
template<typename Resource>
class ResourceLifecycle : public LifecycleState<ResourceState, Resource> {
public:
    /**
     * @brief Construct a new ResourceLifecycle
     */
    ResourceLifecycle()
        : LifecycleState<ResourceState, Resource>(ResourceState::Unloaded) {}
    
    /**
     * @brief Virtual destructor
     */
    virtual ~ResourceLifecycle() = default;
    
    /**
     * @brief Load the resource
     * 
     * This method attempts to load the resource by transitioning to the Loading
     * state, calling loadImpl(), and then transitioning to the Loaded or LoadingFailed
     * state depending on the result.
     * 
     * If the resource is already loaded, it increments the load count and returns true.
     * 
     * @return true if loading was successful, false otherwise
     */
    bool load() {
        // Check if the resource is already loaded
        if (this->getState() == ResourceState::Loaded) {
            // Increment the load count and return
            loadCount_++;
            return true;
        }
        
        // Transition to Loading state
        if (!this->transitionTo(ResourceState::Loading)) {
            Logger::logError("Failed to transition to Loading state");
            return false;
        }
        
        // Attempt to load the resource
        bool success = false;
        try {
            success = static_cast<Resource*>(this)->loadImpl();
        } catch (const std::exception& e) {
            Logger::logError("Exception during resource loading: " + std::string(e.what()));
            success = false;
        } catch (...) {
            Logger::logError("Unknown exception during resource loading");
            success = false;
        }
        
        // Transition to the appropriate state based on the result
        if (success) {
            if (this->transitionTo(ResourceState::Loaded)) {
                loadCount_++;
                return true;
            } else {
                Logger::logError("Failed to transition to Loaded state");
                return false;
            }
        } else {
            if (!this->transitionTo(ResourceState::LoadingFailed)) {
                Logger::logError("Failed to transition to LoadingFailed state");
            }
            return false;
        }
    }
    
    /**
     * @brief Unload the resource
     * 
     * This method decrements the load count and unloads the resource when the
     * count reaches zero. It transitions to the Unloading state, calls unloadImpl(),
     * and then transitions to the Unloaded state.
     */
    void unload() {
        // Check if the resource is already unloaded
        if (this->getState() == ResourceState::Unloaded) {
            return;
        }
        
        // Decrement the load count
        if (loadCount_ > 0) {
            loadCount_--;
        }
        
        // If we still have references, don't actually unload
        if (loadCount_ > 0) {
            return;
        }
        
        // Transition to Unloading state
        if (!this->transitionTo(ResourceState::Unloading)) {
            Logger::logError("Failed to transition to Unloading state");
            return;
        }
        
        // Attempt to unload the resource
        try {
            static_cast<Resource*>(this)->unloadImpl();
        } catch (const std::exception& e) {
            Logger::logError("Exception during resource unloading: " + std::string(e.what()));
        } catch (...) {
            Logger::logError("Unknown exception during resource unloading");
        }
        
        // Transition to Unloaded state
        if (!this->transitionTo(ResourceState::Unloaded)) {
            Logger::logError("Failed to transition to Unloaded state");
        }
    }
    
    /**
     * @brief Get the current load count
     * 
     * @return The number of times the resource has been loaded without being unloaded
     */
    int getLoadCount() const {
        return loadCount_.load();
    }
    
    /**
     * @brief Check if a transition from one state to another is valid
     * 
     * This method defines the valid state transitions for resources:
     * - Unloaded -> Loading
     * - Loading -> Loaded or LoadingFailed
     * - Loaded -> Unloading
     * - LoadingFailed -> Loading or Unloaded
     * - Unloading -> Unloaded
     * 
     * @param from The current state
     * @param to The target state
     * @return true if the transition is valid, false otherwise
     */
    bool isValidTransition(ResourceState from, ResourceState to) const override {
        switch (from) {
            case ResourceState::Unloaded:
                return to == ResourceState::Loading;
                
            case ResourceState::Loading:
                return to == ResourceState::Loaded || 
                       to == ResourceState::LoadingFailed;
                
            case ResourceState::Loaded:
                return to == ResourceState::Unloading;
                
            case ResourceState::LoadingFailed:
                return to == ResourceState::Loading || 
                       to == ResourceState::Unloaded;
                
            case ResourceState::Unloading:
                return to == ResourceState::Unloaded;
                
            default:
                return false;
        }
    }
    
protected:
    /**
     * @brief Implementation of resource loading
     * 
     * Derived classes must implement this method to define how the resource
     * is actually loaded.
     * 
     * @return true if loading was successful, false otherwise
     */
    virtual bool loadImpl() = 0;
    
    /**
     * @brief Implementation of resource unloading
     * 
     * Derived classes must implement this method to define how the resource
     * is actually unloaded.
     */
    virtual void unloadImpl() = 0;
    
private:
    std::atomic<int> loadCount_{0};
};

/**
 * @brief Convert a ResourceState to string for logging and debugging
 * 
 * @param state The resource state
 * @return String representation of the state
 */
inline std::string resourceStateToString(ResourceState state) {
    switch (state) {
        case ResourceState::Unloaded:      return "Unloaded";
        case ResourceState::Loading:       return "Loading";
        case ResourceState::Loaded:        return "Loaded";
        case ResourceState::LoadingFailed: return "LoadingFailed";
        case ResourceState::Unloading:     return "Unloading";
        default:                           return "Unknown";
    }
}

} // namespace Utils
} // namespace Fabric