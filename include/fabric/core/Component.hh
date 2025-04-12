#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <variant>
#include <mutex>
#include <optional>

namespace Fabric {

/**
 * @brief Base class for all UI components in the Fabric framework
 * 
 * The Component class provides the foundation for creating UI elements 
 * within the Fabric framework. It defines the common interface that all
 * components must implement, including lifecycle methods, event handling,
 * and property management.
 */
class Component {
public:
  /**
   * @brief Supported property value types
   * 
   * This variant defines all types that can be stored in component properties.
   * To add support for additional types, extend this variant definition.
   */
  using PropertyValue = std::variant<
    bool,
    int,
    float,
    double,
    std::string,
    std::shared_ptr<Component>
  >;

  /**
   * @brief Component constructor
   * 
   * @param id Unique identifier for the component
   * @throws FabricException if id is empty
   */
  explicit Component(const std::string& id);
  
  /**
   * @brief Virtual destructor
   */
  virtual ~Component() = default;

  /**
   * @brief Get the component's unique identifier
   * 
   * @return Component ID
   */
  const std::string& getId() const;

  /**
   * @brief Initialize the component
   * 
   * This method is called after the component is created but before
   * it is rendered for the first time. Use this method to perform any
   * initialization tasks.
   */
  virtual void initialize() = 0;

  /**
   * @brief Render the component
   * 
   * This method is called when the component needs to be rendered.
   * It should return a string representation of the component.
   * 
   * @return String representation of the component
   */
  virtual std::string render() = 0;

  /**
   * @brief Update the component
   * 
   * This method is called when the component needs to be updated.
   * Override this method to implement custom update logic.
   * 
   * @param deltaTime Time elapsed since the last update in seconds
   */
  virtual void update(float deltaTime) = 0;

  /**
   * @brief Clean up component resources
   * 
   * This method is called before the component is destroyed.
   * Override this method to perform any cleanup tasks.
   */
  virtual void cleanup() = 0;

  /**
   * @brief Set a property value
   * 
   * @tparam T Type of the property value (must be one of the types in PropertyValue)
   * @param name Property name
   * @param value Property value
   */
  template <typename T>
  void setProperty(const std::string& name, const T& value);

  /**
   * @brief Get a property value
   * 
   * @tparam T Expected type of the property value
   * @param name Property name
   * @return Property value
   * @throws FabricException if property doesn't exist or is wrong type
   */
  template <typename T>
  T getProperty(const std::string& name) const;

  /**
   * @brief Check if a property exists
   * 
   * @param name Property name
   * @return true if the property exists, false otherwise
   */
  bool hasProperty(const std::string& name) const;

  /**
   * @brief Remove a property
   * 
   * @param name Property name to remove
   * @return true if the property was removed, false if it didn't exist
   */
  bool removeProperty(const std::string& name);

  /**
   * @brief Add a child component
   * 
   * @param child Child component to add
   * @throws FabricException if child is null or if a child with the same ID already exists
   */
  void addChild(std::shared_ptr<Component> child);

  /**
   * @brief Remove a child component
   * 
   * @param childId ID of the child component to remove
   * @return true if the child was removed, false otherwise
   */
  bool removeChild(const std::string& childId);

  /**
   * @brief Get a child component by ID
   * 
   * @param childId ID of the child component to get
   * @return Child component or nullptr if not found
   */
  std::shared_ptr<Component> getChild(const std::string& childId) const;

  /**
   * @brief Get all child components
   * 
   * @return Vector of child components
   */
  const std::vector<std::shared_ptr<Component>>& getChildren() const;

private:
  std::string id;
  mutable std::mutex propertiesMutex;  // Mutex for thread-safe property access
  std::unordered_map<std::string, PropertyValue> properties;
  
  mutable std::mutex childrenMutex;    // Mutex for thread-safe children access
  std::vector<std::shared_ptr<Component>> children;
};

} // namespace Fabric