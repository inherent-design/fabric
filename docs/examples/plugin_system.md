# Plugin System Integration

[← Back to Examples Index](../EXAMPLES.md)

This example demonstrates how to create and register plugins to extend the Fabric Engine:

```cpp
#include "fabric/core/Plugin.hh"
#include "fabric/core/Component.hh"
#include "fabric/utils/Logging.hh"

using namespace Fabric;

// Custom components provided by the plugin
class CustomButton : public Component {
public:
  CustomButton(const std::string& id) : Component(id) {
    setProperty<std::string>("label", "Custom Button");
    setProperty<std::string>("theme", "modern");
  }
  
  void initialize() override {
    Logger::logInfo("CustomButton " + getId() + " initialized");
  }
  
  std::string render() override {
    std::string label = getProperty<std::string>("label");
    std::string theme = getProperty<std::string>("theme");
    
    std::string html = "<button class='custom-button theme-" + theme + "'>";
    html += label;
    html += "</button>";
    
    return html;
  }
  
  void update(float deltaTime) override {}
  void cleanup() override {}
};

class CustomInput : public Component {
public:
  CustomInput(const std::string& id) : Component(id) {
    setProperty<std::string>("placeholder", "Type here...");
    setProperty<std::string>("theme", "modern");
  }
  
  void initialize() override {
    Logger::logInfo("CustomInput " + getId() + " initialized");
  }
  
  std::string render() override {
    std::string placeholder = getProperty<std::string>("placeholder");
    std::string theme = getProperty<std::string>("theme");
    
    std::string html = "<input class='custom-input theme-" + theme + "' ";
    html += "placeholder='" + placeholder + "' />";
    
    return html;
  }
  
  void update(float deltaTime) override {}
  void cleanup() override {}
};

// Define the plugin
class UIExtensionPlugin : public Plugin {
public:
  std::string getName() const override {
    return "UIExtensionPlugin";
  }
  
  std::string getVersion() const override {
    return "1.0.0";
  }
  
  std::string getAuthor() const override {
    return "Fabric Team";
  }
  
  std::string getDescription() const override {
    return "Provides additional UI components with custom themes";
  }
  
  bool initialize() override {
    Logger::logInfo("Initializing " + getName() + " plugin");
    
    // Register CSS styles that the plugin provides
    registerStyles();
    
    return true;
  }
  
  void shutdown() override {
    Logger::logInfo("Shutting down " + getName() + " plugin");
    
    // Clean up any resources used by the plugin
    customComponents.clear();
  }
  
  std::vector<std::shared_ptr<Component>> getComponents() override {
    // If this is the first call, create the components
    if (customComponents.empty()) {
      customComponents.push_back(std::make_shared<CustomButton>("custom-button-1"));
      customComponents.push_back(std::make_shared<CustomInput>("custom-input-1"));
    }
    
    return customComponents;
  }
  
private:
  std::vector<std::shared_ptr<Component>> customComponents;
  
  void registerStyles() {
    // In a real implementation, this might inject CSS into the WebView
    Logger::logInfo("Registering custom styles for " + getName());
  }
};

// Register the plugin with the plugin manager
FABRIC_REGISTER_PLUGIN(UIExtensionPlugin)

// Example of using the plugin system
void pluginSystemExample() {
  // Get the plugin manager
  auto& pluginManager = PluginManager::getInstance();
  
  // Load the UI extension plugin
  bool success = pluginManager.loadPlugin("UIExtensionPlugin");
  if (success) {
    std::cout << "Plugin loaded successfully" << std::endl;
    
    // Get the plugin
    auto plugin = pluginManager.getPlugin("UIExtensionPlugin");
    if (plugin) {
      std::cout << "Plugin: " << plugin->getName() << " v" << 
                   plugin->getVersion() << std::endl;
      std::cout << "Author: " << plugin->getAuthor() << std::endl;
      std::cout << "Description: " << plugin->getDescription() << std::endl;
      
      // Get components provided by the plugin
      auto components = plugin->getComponents();
      std::cout << "Plugin provides " << components.size() << 
                   " components:" << std::endl;
      
      for (const auto& component : components) {
        std::cout << "- " << component->getId() << std::endl;
        
        // Initialize and render the component
        component->initialize();
        std::string html = component->render();
        std::cout << "  HTML: " << html << std::endl;
      }
    }
    
    // Unload the plugin when done
    pluginManager.unloadPlugin("UIExtensionPlugin");
    std::cout << "Plugin unloaded" << std::endl;
  } else {
    std::cout << "Failed to load plugin" << std::endl;
  }
}
```

[← Back to Examples Index](../EXAMPLES.md)