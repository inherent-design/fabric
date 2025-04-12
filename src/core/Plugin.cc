#include "fabric/core/Plugin.hh"
#include "fabric/utils/ErrorHandling.hh"
#include "fabric/utils/Logging.hh"
#include <algorithm>
#include <vector>

namespace Fabric {

PluginManager& PluginManager::getInstance() {
  static PluginManager instance;
  return instance;
}

void PluginManager::registerPlugin(const std::string& name, const PluginFactory& factory) {
  std::lock_guard<std::mutex> lock(pluginMutex);
  
  if (name.empty()) {
    throwError("Plugin name cannot be empty");
  }
  
  if (!factory) {
    throwError("Plugin factory cannot be null");
  }
  
  if (pluginFactories.find(name) != pluginFactories.end()) {
    throwError("Plugin '" + name + "' is already registered");
  }
  
  pluginFactories[name] = factory;
  Logger::logDebug("Registered plugin '" + name + "'");
}

bool PluginManager::loadPlugin(const std::string& name) {
  std::lock_guard<std::mutex> lock(pluginMutex);
  
  // Check if already loaded
  if (loadedPlugins.find(name) != loadedPlugins.end()) {
    Logger::logWarning("Plugin '" + name + "' is already loaded");
    return true;
  }
  
  // Find factory
  auto factoryIt = pluginFactories.find(name);
  if (factoryIt == pluginFactories.end()) {
    Logger::logError("Plugin '" + name + "' is not registered");
    return false;
  }
  
  try {
    // Create plugin instance
    auto plugin = factoryIt->second();
    if (!plugin) {
      Logger::logError("Failed to create plugin '" + name + "'");
      return false;
    }
    
    // Add to loaded plugins
    loadedPlugins[name] = plugin;
    Logger::logInfo("Loaded plugin '" + name + "' (" + plugin->getVersion() + 
                   ") by " + plugin->getAuthor());
    
    return true;
  } catch (const std::exception& e) {
    Logger::logError("Exception loading plugin '" + name + "': " + std::string(e.what()));
    return false;
  } catch (...) {
    Logger::logError("Unknown exception loading plugin '" + name + "'");
    return false;
  }
}

bool PluginManager::unloadPlugin(const std::string& name) {
  std::shared_ptr<Plugin> pluginToUnload;
  
  {
    std::lock_guard<std::mutex> lock(pluginMutex);
    
    auto it = loadedPlugins.find(name);
    if (it == loadedPlugins.end()) {
      Logger::logWarning("Plugin '" + name + "' is not loaded");
      return false;
    }
    
    // Store the plugin to unload outside the lock
    pluginToUnload = it->second;
    
    // Remove from loaded plugins immediately to prevent cyclic dependencies
    loadedPlugins.erase(it);
  }
  
  try {
    // Shut down the plugin outside the lock to prevent deadlocks
    if (pluginToUnload) {
      pluginToUnload->shutdown();
    }
    
    Logger::logInfo("Unloaded plugin '" + name + "'");
    return true;
  } catch (const std::exception& e) {
    Logger::logError("Exception unloading plugin '" + name + "': " + std::string(e.what()));
    return false;
  } catch (...) {
    Logger::logError("Unknown exception unloading plugin '" + name + "'");
    return false;
  }
}

std::shared_ptr<Plugin> PluginManager::getPlugin(const std::string& name) const {
  std::lock_guard<std::mutex> lock(pluginMutex);
  
  auto it = loadedPlugins.find(name);
  if (it == loadedPlugins.end()) {
    return nullptr;
  }
  
  return it->second;
}

std::unordered_map<std::string, std::shared_ptr<Plugin>> PluginManager::getPlugins() const {
  std::lock_guard<std::mutex> lock(pluginMutex);
  return loadedPlugins; // Return a copy for thread safety
}

bool PluginManager::initializeAll() {
  // Create a copy of the plugins to avoid holding the lock during initialization
  std::vector<std::pair<std::string, std::shared_ptr<Plugin>>> plugins;
  
  {
    std::lock_guard<std::mutex> lock(pluginMutex);
    plugins.reserve(loadedPlugins.size());
    for (const auto& pair : loadedPlugins) {
      plugins.push_back(pair);
    }
  }
  
  bool success = true;
  
  for (const auto& [name, plugin] : plugins) {
    if (!plugin) {
      Logger::logError("Null plugin reference for '" + name + "'");
      success = false;
      continue;
    }
    
    try {
      if (!plugin->initialize()) {
        Logger::logError("Failed to initialize plugin '" + name + "'");
        success = false;
      } else {
        Logger::logInfo("Initialized plugin '" + name + "'");
      }
    } catch (const std::exception& e) {
      Logger::logError("Exception initializing plugin '" + name + "': " + std::string(e.what()));
      success = false;
    } catch (...) {
      Logger::logError("Unknown exception initializing plugin '" + name + "'");
      success = false;
    }
  }
  
  return success;
}

void PluginManager::shutdownAll() {
  // Copy all plugins to a vector for shutdown
  // This allows us to control shutdown order and handle dependencies
  std::vector<std::pair<std::string, std::shared_ptr<Plugin>>> plugins;
  
  {
    std::lock_guard<std::mutex> lock(pluginMutex);
    plugins.reserve(loadedPlugins.size());
    for (const auto& pair : loadedPlugins) {
      plugins.push_back(pair);
    }
    
    // Clear the loaded plugins container first to prevent cyclical shutdown dependencies
    loadedPlugins.clear();
  }
  
  // Reverse the order to handle potential dependencies (shutdown in reverse order of loading)
  std::reverse(plugins.begin(), plugins.end());
  
  for (const auto& [name, plugin] : plugins) {
    if (!plugin) {
      Logger::logWarning("Null plugin reference for '" + name + "' during shutdown");
      continue;
    }
    
    try {
      plugin->shutdown();
      Logger::logInfo("Shut down plugin '" + name + "'");
    } catch (const std::exception& e) {
      Logger::logError("Exception shutting down plugin '" + name + "': " + std::string(e.what()));
    } catch (...) {
      Logger::logError("Unknown exception shutting down plugin '" + name + "'");
    }
  }
}

} // namespace Fabric