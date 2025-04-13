# Component Lifecycle Management

[← Back to Examples Index](../EXAMPLES.md)

This example demonstrates how to use the lifecycle manager to control component state transitions:

```cpp
#include "fabric/core/Component.hh"
#include "fabric/core/Lifecycle.hh"
#include "fabric/utils/Logging.hh"

using namespace Fabric;

// Component with detailed lifecycle management
class PageView : public Component {
public:
  PageView(const std::string& id) : Component(id), lifecycleManager() {
    // Register hooks for different lifecycle states
    lifecycleManager.addHook(LifecycleState::Initialized, [this]() {
      Logger::logInfo("PageView " + getId() + " initialized");
      onInitialized();
    });
    
    lifecycleManager.addHook(LifecycleState::Rendered, [this]() {
      Logger::logInfo("PageView " + getId() + " rendered");
      onFirstRender();
    });
    
    lifecycleManager.addTransitionHook(
      LifecycleState::Rendered, 
      LifecycleState::Updating, 
      [this]() {
        Logger::logInfo("PageView " + getId() + " entering update loop");
        onBeginUpdates();
      }
    );
    
    lifecycleManager.addTransitionHook(
      LifecycleState::Updating, 
      LifecycleState::Suspended, 
      [this]() {
        Logger::logInfo("PageView " + getId() + " suspending");
        onSuspend();
      }
    );
    
    lifecycleManager.addTransitionHook(
      LifecycleState::Suspended, 
      LifecycleState::Updating, 
      [this]() {
        Logger::logInfo("PageView " + getId() + " resuming");
        onResume();
      }
    );
    
    lifecycleManager.addHook(LifecycleState::Destroyed, [this]() {
      Logger::logInfo("PageView " + getId() + " destroyed");
      onDestroyed();
    });
  }
  
  // Override Component lifecycle methods
  void initialize() override {
    lifecycleManager.setState(LifecycleState::Initialized);
    
    // Load any initial data
    loadData();
  }
  
  std::string render() override {
    std::string html = "<div id='" + getId() + "' class='page-view'>";
    
    // Render content based on current state
    if (hasProperty("data")) {
      auto& data = getProperty<std::vector<std::string>>("data");
      for (const auto& item : data) {
        html += "<div class='item'>" + item + "</div>";
      }
    } else {
      html += "<div class='loading'>Loading...</div>";
    }
    
    html += "</div>";
    
    // If this is the first render, update lifecycle state
    if (lifecycleManager.getState() == LifecycleState::Initialized) {
      lifecycleManager.setState(LifecycleState::Rendered);
    }
    
    return html;
  }
  
  void update(float deltaTime) override {
    // If not in updating state, move to it
    if (lifecycleManager.getState() != LifecycleState::Updating) {
      lifecycleManager.setState(LifecycleState::Updating);
    }
    
    // Only update if in correct state
    if (lifecycleManager.getState() == LifecycleState::Updating) {
      updateAnimations(deltaTime);
    }
  }
  
  void cleanup() override {
    lifecycleManager.setState(LifecycleState::Destroyed);
  }
  
  // Additional lifecycle control methods
  void suspend() {
    if (lifecycleManager.getState() == LifecycleState::Updating) {
      lifecycleManager.setState(LifecycleState::Suspended);
    }
  }
  
  void resume() {
    if (lifecycleManager.getState() == LifecycleState::Suspended) {
      lifecycleManager.setState(LifecycleState::Updating);
    }
  }
  
private:
  LifecycleManager lifecycleManager;
  
  // Lifecycle event handlers
  void onInitialized() {
    setProperty<bool>("isInitialized", true);
  }
  
  void onFirstRender() {
    setProperty<bool>("isVisible", true);
  }
  
  void onBeginUpdates() {
    setProperty<bool>("isActive", true);
  }
  
  void onSuspend() {
    setProperty<bool>("isActive", false);
    // Save state if needed
    saveState();
  }
  
  void onResume() {
    setProperty<bool>("isActive", true);
    // Restore state if needed
    restoreState();
  }
  
  void onDestroyed() {
    // Clean up any resources
    clearData();
  }
  
  // Example helper methods
  void loadData() {
    // Simulate loading data
    std::vector<std::string> data = {"Item 1", "Item 2", "Item 3"};
    setProperty<std::vector<std::string>>("data", data);
  }
  
  void updateAnimations(float deltaTime) {
    // Example animation update
    if (hasProperty("animationProgress")) {
      float progress = getProperty<float>("animationProgress");
      progress += deltaTime / 2.0f; // 2 second animation
      if (progress > 1.0f) {
        progress = 1.0f;
      }
      setProperty<float>("animationProgress", progress);
    }
  }
  
  void saveState() {
    Logger::logInfo("Saving state for " + getId());
    // Save any state that needs to persist during suspension
  }
  
  void restoreState() {
    Logger::logInfo("Restoring state for " + getId());
    // Restore any saved state after resuming
  }
  
  void clearData() {
    if (hasProperty("data")) {
      removeProperty("data");
    }
  }
};

// Example of lifecycle management usage
void lifecycleManagementExample() {
  // Create a page view component
  auto pageView = std::make_shared<PageView>("main-page");
  
  // Initialize the component (triggers Initialized state)
  pageView->initialize();
  
  // Render the component (triggers Rendered state)
  std::string html = pageView->render();
  std::cout << "Initial render: " << html << std::endl;
  
  // Start update loop (triggers Updating state)
  for (int i = 0; i < 5; i++) {
    pageView->update(0.1f);
    std::cout << "Update " << i << std::endl;
  }
  
  // Suspend the component (triggers Suspended state)
  pageView->suspend();
  std::cout << "Component suspended" << std::endl;
  
  // This update should not have an effect while suspended
  pageView->update(0.1f);
  
  // Resume the component (triggers Updating state again)
  pageView->resume();
  std::cout << "Component resumed" << std::endl;
  
  // More updates after resuming
  for (int i = 0; i < 3; i++) {
    pageView->update(0.1f);
    std::cout << "Update after resume " << i << std::endl;
  }
  
  // Cleanup the component (triggers Destroyed state)
  pageView->cleanup();
}
```

[← Back to Examples Index](../EXAMPLES.md)