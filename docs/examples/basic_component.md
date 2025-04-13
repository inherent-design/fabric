# Basic Component Structure

[← Back to Examples Index](../EXAMPLES.md)

The following example shows how to create a basic component with properties and rendering capabilities:

```cpp
#include "fabric/core/Component.hh"
#include "fabric/utils/Logging.hh"

using namespace Fabric;

// Define a custom component
class Button : public Component {
public:
  Button(const std::string& id) : Component(id) {
    // Initialize default properties
    setProperty<std::string>("label", "Click Me");
    setProperty<bool>("enabled", true);
    setProperty<std::string>("backgroundColor", "#3498db");
    setProperty<std::string>("textColor", "#ffffff");
  }

  void initialize() override {
    Logger::logInfo("Button " + getId() + " initialized");
    
    // Initialize any resources needed by the button
    // This is called once before the first render
  }

  std::string render() override {
    std::string label = getProperty<std::string>("label");
    bool enabled = getProperty<bool>("enabled");
    std::string bgColor = getProperty<std::string>("backgroundColor");
    std::string textColor = getProperty<std::string>("textColor");
    
    // Construct HTML representation (for WebView rendering)
    std::string buttonHTML = "<button id='" + getId() + "' ";
    buttonHTML += "style='background-color: " + bgColor + "; color: " + textColor + ";' ";
    
    if (!enabled) {
      buttonHTML += "disabled ";
    }
    
    buttonHTML += "onclick='fabricBridge.triggerEvent(\"" + getId() + "\", \"click\")'>";
    buttonHTML += label;
    buttonHTML += "</button>";
    
    return buttonHTML;
  }

  void update(float deltaTime) override {
    // Update component state
    // This is called each frame (if active)
    
    // Example: Animate a property
    if (hasProperty("animating") && getProperty<bool>("animating")) {
      float currentOpacity = getProperty<float>("opacity");
      float newOpacity = currentOpacity + (0.5f * deltaTime); // Fade in over 2 seconds
      if (newOpacity > 1.0f) {
        newOpacity = 1.0f;
        setProperty<bool>("animating", false);
      }
      setProperty<float>("opacity", newOpacity);
    }
  }

  void cleanup() override {
    Logger::logInfo("Button " + getId() + " cleaning up");
    // Release any resources used by the button
  }
};

// Example usage
void buttonComponentExample() {
  // Create a button
  auto button = std::make_shared<Button>("submit-button");
  
  // Customize properties
  button->setProperty<std::string>("label", "Submit Form");
  button->setProperty<std::string>("backgroundColor", "#27ae60");
  
  // Initialize the button
  button->initialize();
  
  // Render the button (get its HTML representation)
  std::string html = button->render();
  std::cout << "Button HTML: " << html << std::endl;
  
  // Start animation
  button->setProperty<float>("opacity", 0.0f);
  button->setProperty<bool>("animating", true);
  
  // Simulate a few update frames
  for (int i = 0; i < 10; i++) {
    button->update(0.1f); // 100ms between frames
    std::cout << "Opacity: " << button->getProperty<float>("opacity") << std::endl;
  }
  
  // Clean up the button
  button->cleanup();
}
```

[← Back to Examples Index](../EXAMPLES.md)