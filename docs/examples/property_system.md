# Type-Safe Property System

[← Back to Examples Index](../EXAMPLES.md)

This example demonstrates how to use the type-safe property system for components:

```cpp
#include "fabric/core/Component.hh"
#include "fabric/utils/Logging.hh"
#include <iostream>

using namespace Fabric;

// Define a component that makes extensive use of properties
class ConfigurablePanel : public Component {
public:
  ConfigurablePanel(const std::string& id) : Component(id) {
    // Initialize with default properties
    setProperty<std::string>("title", "Panel");
    setProperty<int>("width", 300);
    setProperty<int>("height", 200);
    setProperty<bool>("collapsible", true);
    setProperty<bool>("collapsed", false);
    setProperty<std::string>("backgroundColor", "#ffffff");
    setProperty<float>("borderRadius", 4.0f);
    setProperty<std::string>("fontFamily", "Arial, sans-serif");
    setProperty<std::vector<std::string>>("tags", {"panel", "UI"});
  }
  
  void initialize() override {}
  void update(float deltaTime) override {}
  void cleanup() override {}
  
  std::string render() override {
    // Extract properties with type safety
    std::string title = getProperty<std::string>("title");
    int width = getProperty<int>("width");
    int height = getProperty<int>("height");
    bool collapsible = getProperty<bool>("collapsible");
    bool collapsed = getProperty<bool>("collapsed");
    std::string bgColor = getProperty<std::string>("backgroundColor");
    float borderRadius = getProperty<float>("borderRadius");
    std::string fontFamily = getProperty<std::string>("fontFamily");
    
    // Begin constructing HTML
    std::string styleAttr = "style='";
    styleAttr += "width: " + std::to_string(width) + "px; ";
    
    if (!collapsed) {
      styleAttr += "height: " + std::to_string(height) + "px; ";
    } else {
      styleAttr += "height: 40px; "; // Collapsed height
    }
    
    styleAttr += "background-color: " + bgColor + "; ";
    styleAttr += "border-radius: " + std::to_string(borderRadius) + "px; ";
    styleAttr += "font-family: " + fontFamily + "; ";
    styleAttr += "overflow: hidden;'";
    
    std::string html = "<div id='" + getId() + "' class='panel' " + styleAttr + ">";
    
    // Add header
    html += "<div class='panel-header'>";
    html += "<h3>" + title + "</h3>";
    
    if (collapsible) {
      std::string collapseIcon = collapsed ? "+" : "-";
      html += "<button class='collapse-btn'>" + collapseIcon + "</button>";
    }
    
    html += "</div>";
    
    // Add content (only if not collapsed)
    if (!collapsed) {
      html += "<div class='panel-content'>";
      
      // Check if we have content to display
      if (hasProperty("content")) {
        html += getProperty<std::string>("content");
      } else {
        html += "<p>No content to display</p>";
      }
      
      html += "</div>";
    }
    
    html += "</div>";
    return html;
  }
  
  // Type-safe helper methods for specific properties
  void setTitle(const std::string& title) {
    setProperty<std::string>("title", title);
  }
  
  std::string getTitle() const {
    return getProperty<std::string>("title");
  }
  
  void setDimensions(int width, int height) {
    setProperty<int>("width", width);
    setProperty<int>("height", height);
  }
  
  void setCollapsed(bool collapsed) {
    setProperty<bool>("collapsed", collapsed);
  }
  
  bool isCollapsed() const {
    return getProperty<bool>("collapsed");
  }
  
  void toggleCollapsed() {
    setCollapsed(!isCollapsed());
  }
  
  void setContent(const std::string& content) {
    setProperty<std::string>("content", content);
  }
  
  void addTag(const std::string& tag) {
    // Get current tags
    auto tags = getProperty<std::vector<std::string>>("tags");
    
    // Check if tag already exists
    if (std::find(tags.begin(), tags.end(), tag) == tags.end()) {
      tags.push_back(tag);
      setProperty<std::vector<std::string>>("tags", tags);
    }
  }
  
  std::vector<std::string> getTags() const {
    return getProperty<std::vector<std::string>>("tags");
  }
};

// Example usage of the property system
void propertySystemExample() {
  // Create a configurable panel
  auto panel = std::make_shared<ConfigurablePanel>("settings-panel");
  
  // Use the helper methods to set properties
  panel->setTitle("Application Settings");
  panel->setDimensions(400, 300);
  panel->setContent("<p>Configure your application settings here.</p>");
  
  // Add some tags
  panel->addTag("settings");
  panel->addTag("configuration");
  
  // Render the panel
  std::string html = panel->render();
  std::cout << "Panel HTML: " << html << std::endl;
  
  // Display the panel's tags
  std::cout << "Panel tags: ";
  for (const auto& tag : panel->getTags()) {
    std::cout << tag << " ";
  }
  std::cout << std::endl;
  
  // Toggle collapse state
  std::cout << "Initially collapsed: " << 
    (panel->isCollapsed() ? "Yes" : "No") << std::endl;
  
  panel->toggleCollapsed();
  std::cout << "After toggle, collapsed: " << 
    (panel->isCollapsed() ? "Yes" : "No") << std::endl;
  
  // Re-render after state change
  html = panel->render();
  std::cout << "Panel HTML after collapse: " << html << std::endl;
  
  // Direct property access (avoid when possible, prefer helper methods)
  panel->setProperty<std::string>("backgroundColor", "#f0f0f0");
  panel->setProperty<float>("borderRadius", 8.0f);
  
  // Invalid property access (would throw an exception)
  try {
    int invalid = panel->getProperty<int>("title"); // Title is a string
    std::cout << "This should not print: " << invalid << std::endl;
  } catch (const std::exception& e) {
    std::cout << "Expected error: " << e.what() << std::endl;
  }
}
```

[← Back to Examples Index](../EXAMPLES.md)