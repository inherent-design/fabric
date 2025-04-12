#pragma once

#include "fabric/core/Component.hh"
#include "fabric/core/Event.hh"
#include "fabric/core/Lifecycle.hh"
#include <memory>
#include <string>

namespace Fabric {
namespace Testing {

/**
 * @brief Mock component for testing
 */
class MockComponent : public Component {
public:
  explicit MockComponent(const std::string& id) : Component(id) {}

  void initialize() override {}
  std::string render() override { return "<mock-component id=\"" + getId() + "\"></mock-component>"; }
  void update(float deltaTime) override {}
  void cleanup() override {}

  // Test helpers
  int initializeCallCount = 0;
  int renderCallCount = 0;
  int updateCallCount = 0;
  int cleanupCallCount = 0;

  // Override these methods for testing
  void initialize_impl() { initializeCallCount++; }
  std::string render_impl() { renderCallCount++; return ""; }
  void update_impl(float deltaTime) { updateCallCount++; }
  void cleanup_impl() { cleanupCallCount++; }
};

/**
 * @brief Event recorder for testing event dispatch
 */
class EventRecorder {
public:
  /**
   * @brief Record an event
   * 
   * @param event The event to record
   */
  void recordEvent(const Event& event) {
    lastEventType = event.getType();
    lastEventSource = event.getSource();
    eventCount++;
  }

  /**
   * @brief Get a handler function for the event recorder
   * 
   * @return Event handler function
   */
  EventHandler getHandler() {
    return [this](const Event& event) {
      this->recordEvent(event);
    };
  }

  /**
   * @brief Reset the recorder
   */
  void reset() {
    lastEventType = "";
    lastEventSource = "";
    eventCount = 0;
  }

  // Record data
  std::string lastEventType;
  std::string lastEventSource;
  int eventCount = 0;
};

/**
 * @brief Lifecycle recorder for testing lifecycle transitions
 */
class LifecycleRecorder {
public:
  /**
   * @brief Record a lifecycle state change
   * 
   * @param state The new state
   */
  void recordState(LifecycleState state) {
    lastState = state;
    stateChanges++;
  }

  /**
   * @brief Get a handler function for the lifecycle recorder
   * 
   * @return Lifecycle hook function
   */
  LifecycleHook getHook() {
    return [this]() {
      this->stateChanges++;
    };
  }

  /**
   * @brief Get a transition handler function for the lifecycle recorder
   * 
   * @param fromState The from state
   * @param toState The to state
   * @return Lifecycle hook function
   */
  LifecycleHook getTransitionHook(LifecycleState fromState, LifecycleState toState) {
    return [this, fromState, toState]() {
      this->lastFromState = fromState;
      this->lastToState = toState;
      this->transitionChanges++;
    };
  }

  /**
   * @brief Reset the recorder
   */
  void reset() {
    lastState = LifecycleState::Created;
    lastFromState = LifecycleState::Created;
    lastToState = LifecycleState::Created;
    stateChanges = 0;
    transitionChanges = 0;
  }

  // Record data
  LifecycleState lastState = LifecycleState::Created;
  LifecycleState lastFromState = LifecycleState::Created;
  LifecycleState lastToState = LifecycleState::Created;
  int stateChanges = 0;
  int transitionChanges = 0;
};

} // namespace Testing
} // namespace Fabric