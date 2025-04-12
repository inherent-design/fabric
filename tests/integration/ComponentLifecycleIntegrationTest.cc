#include "fabric/core/Component.hh"
#include "fabric/core/Event.hh"
#include "fabric/core/Lifecycle.hh"
#include "fabric/utils/ErrorHandling.hh"
#include "fabric/utils/Logging.hh"
#include "fabric/utils/Testing.hh"
#include <gtest/gtest.h>
#include <memory>
#include <string>

using namespace Fabric;
using namespace Fabric::Testing;

/**
 * @brief Integration test for Component/Lifecycle/Event interaction
 * 
 * This test demonstrates how components, lifecycle management, and events
 * work together in real application scenarios.
 */
class ComponentLifecycleIntegrationTest : public ::testing::Test {
protected:
  // Test component that tracks its lifecycle
  class TrackedComponent : public MockComponent {
  public:
    explicit TrackedComponent(const std::string& id) 
        : MockComponent(id), lifecycleManager(std::make_unique<LifecycleManager>()) {
      
      // Set up lifecycle tracking
      lifecycleManager->addHook(LifecycleState::Initialized, [this]() {
        lifecycleEvents.push_back("Initialized");
        initialize_impl();
      });
      
      lifecycleManager->addHook(LifecycleState::Rendered, [this]() {
        lifecycleEvents.push_back("Rendered");
      });
      
      lifecycleManager->addHook(LifecycleState::Updating, [this]() {
        lifecycleEvents.push_back("Updating");
      });
      
      lifecycleManager->addHook(LifecycleState::Suspended, [this]() {
        lifecycleEvents.push_back("Suspended");
      });
      
      lifecycleManager->addHook(LifecycleState::Destroyed, [this]() {
        lifecycleEvents.push_back("Destroyed");
        cleanup_impl();
      });
    }
    
    void initialize() override {
      lifecycleManager->setState(LifecycleState::Initialized);
    }
    
    void update(float deltaTime) override {
      if (lifecycleManager->getState() == LifecycleState::Rendered ||
          lifecycleManager->getState() == LifecycleState::Updating) {
        lifecycleManager->setState(LifecycleState::Updating);
        update_impl(deltaTime);
        lifecycleManager->setState(LifecycleState::Rendered);
      }
    }
    
    std::string render() override {
      if (lifecycleManager->getState() == LifecycleState::Initialized ||
          lifecycleManager->getState() == LifecycleState::Rendered ||
          lifecycleManager->getState() == LifecycleState::Updating) {
        lifecycleManager->setState(LifecycleState::Rendered);
        auto result = render_impl();
        lastRenderResult = result;
      }
      return lastRenderResult;
    }
    
    void cleanup() override {
      lifecycleManager->setState(LifecycleState::Destroyed);
    }
    
    void suspend() {
      if (lifecycleManager->getState() != LifecycleState::Created &&
          lifecycleManager->getState() != LifecycleState::Destroyed) {
        lifecycleManager->setState(LifecycleState::Suspended);
      }
    }
    
    void resume() {
      if (lifecycleManager->getState() == LifecycleState::Suspended) {
        lifecycleManager->setState(LifecycleState::Initialized);
      }
    }
    
    LifecycleState getState() const {
      return lifecycleManager->getState();
    }
    
    // Testing helpers
    std::vector<std::string> lifecycleEvents;
    std::string lastRenderResult;
    
  private:
    std::unique_ptr<LifecycleManager> lifecycleManager;
  };
  
  void SetUp() override {
    // Create components and event dispatcher
    rootComponent = std::make_shared<TrackedComponent>("root");
    child1 = std::make_shared<TrackedComponent>("child1");
    child2 = std::make_shared<TrackedComponent>("child2");
    
    // Build component tree
    rootComponent->addChild(child1);
    rootComponent->addChild(child2);
    
    // Set up event dispatcher
    dispatcher = std::make_unique<EventDispatcher>();
    
    // Register event listeners
    dispatcher->addEventListener("component-initialized", [this](const Event& event) {
      componentEvents.push_back("Initialized: " + event.getSource());
    });
    
    dispatcher->addEventListener("component-rendered", [this](const Event& event) {
      componentEvents.push_back("Rendered: " + event.getSource());
    });
    
    dispatcher->addEventListener("component-destroyed", [this](const Event& event) {
      componentEvents.push_back("Destroyed: " + event.getSource());
    });
  }
  
  // Helper to initialize a component and emit the proper event
  void initializeComponent(std::shared_ptr<TrackedComponent> component) {
    component->initialize();
    Event event("component-initialized", component->getId());
    dispatcher->dispatchEvent(event);
  }
  
  // Helper to render a component and emit the proper event
  void renderComponent(std::shared_ptr<TrackedComponent> component) {
    component->render();
    Event event("component-rendered", component->getId());
    dispatcher->dispatchEvent(event);
  }
  
  // Helper to destroy a component and emit the proper event
  void destroyComponent(std::shared_ptr<TrackedComponent> component) {
    component->cleanup();
    Event event("component-destroyed", component->getId());
    dispatcher->dispatchEvent(event);
  }
  
  std::shared_ptr<TrackedComponent> rootComponent;
  std::shared_ptr<TrackedComponent> child1;
  std::shared_ptr<TrackedComponent> child2;
  std::unique_ptr<EventDispatcher> dispatcher;
  std::vector<std::string> componentEvents;
};

TEST_F(ComponentLifecycleIntegrationTest, ComponentTreeInitialization) {
  // Initialize all components
  initializeComponent(rootComponent);
  
  for (const auto& child : rootComponent->getChildren()) {
    auto trackedChild = std::dynamic_pointer_cast<TrackedComponent>(child);
    initializeComponent(trackedChild);
  }
  
  // Check lifecycle states
  EXPECT_EQ(rootComponent->getState(), LifecycleState::Initialized);
  EXPECT_EQ(child1->getState(), LifecycleState::Initialized);
  EXPECT_EQ(child2->getState(), LifecycleState::Initialized);
  
  // Check component events
  EXPECT_EQ(componentEvents.size(), 3);
  EXPECT_EQ(componentEvents[0], "Initialized: root");
  
  // The order of the children might vary, so we just check that both were initialized
  EXPECT_TRUE(std::find(componentEvents.begin(), componentEvents.end(), 
                       "Initialized: child1") != componentEvents.end());
  EXPECT_TRUE(std::find(componentEvents.begin(), componentEvents.end(), 
                       "Initialized: child2") != componentEvents.end());
}

TEST_F(ComponentLifecycleIntegrationTest, RenderingCycle) {
  // Initialize all components
  initializeComponent(rootComponent);
  initializeComponent(child1);
  initializeComponent(child2);
  
  // Render all components
  renderComponent(rootComponent);
  renderComponent(child1);
  renderComponent(child2);
  
  // Check lifecycle states
  EXPECT_EQ(rootComponent->getState(), LifecycleState::Rendered);
  EXPECT_EQ(child1->getState(), LifecycleState::Rendered);
  EXPECT_EQ(child2->getState(), LifecycleState::Rendered);
  
  // Check lifecycle events
  EXPECT_EQ(rootComponent->lifecycleEvents.size(), 2); // Initialized, Rendered
  EXPECT_EQ(rootComponent->lifecycleEvents[0], "Initialized");
  EXPECT_EQ(rootComponent->lifecycleEvents[1], "Rendered");
  
  // Update all components
  rootComponent->update(0.016f); // ~60fps
  child1->update(0.016f);
  child2->update(0.016f);
  
  // Should be back to Rendered state after update
  EXPECT_EQ(rootComponent->getState(), LifecycleState::Rendered);
  EXPECT_EQ(child1->getState(), LifecycleState::Rendered);
  EXPECT_EQ(child2->getState(), LifecycleState::Rendered);
  
  // Check that updating was tracked
  EXPECT_EQ(rootComponent->lifecycleEvents.size(), 4); // Initialized, Rendered, Updating, Rendered
  EXPECT_EQ(rootComponent->lifecycleEvents[2], "Updating");
  EXPECT_EQ(rootComponent->lifecycleEvents[3], "Rendered");
}

TEST_F(ComponentLifecycleIntegrationTest, SuspendAndResume) {
  // Initialize all components
  initializeComponent(rootComponent);
  initializeComponent(child1);
  initializeComponent(child2);
  
  // Render components
  renderComponent(rootComponent);
  
  // Suspend the root component
  rootComponent->suspend();
  
  // Check state
  EXPECT_EQ(rootComponent->getState(), LifecycleState::Suspended);
  
  // Try updating while suspended (should not change state)
  rootComponent->update(0.016f);
  EXPECT_EQ(rootComponent->getState(), LifecycleState::Suspended);
  
  // Resume the component
  rootComponent->resume();
  
  // Check state
  EXPECT_EQ(rootComponent->getState(), LifecycleState::Initialized);
  
  // Can render again after resuming
  renderComponent(rootComponent);
  EXPECT_EQ(rootComponent->getState(), LifecycleState::Rendered);
}

TEST_F(ComponentLifecycleIntegrationTest, CleanupProcess) {
  // Initialize and render
  initializeComponent(rootComponent);
  initializeComponent(child1);
  initializeComponent(child2);
  
  renderComponent(rootComponent);
  renderComponent(child1);
  renderComponent(child2);
  
  // Now destroy in reverse order
  destroyComponent(child2);
  destroyComponent(child1);
  destroyComponent(rootComponent);
  
  // Check states
  EXPECT_EQ(rootComponent->getState(), LifecycleState::Destroyed);
  EXPECT_EQ(child1->getState(), LifecycleState::Destroyed);
  EXPECT_EQ(child2->getState(), LifecycleState::Destroyed);
  
  // Check events
  EXPECT_EQ(componentEvents.size(), 9); // 3 init + 3 render + 3 destroy
  
  // Check the last three events are destroy events
  EXPECT_EQ(componentEvents[6], "Destroyed: child2");
  EXPECT_EQ(componentEvents[7], "Destroyed: child1");
  EXPECT_EQ(componentEvents[8], "Destroyed: root");
}

