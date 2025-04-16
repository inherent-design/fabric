#pragma once

#include "fabric/core/Types.hh"
#include <any>
#include <functional>
#include <unordered_map>
#include <vector>
#include <mutex>

namespace Fabric {

/**
 * @brief Base class for all events in the Fabric framework
 * 
 * Events are used to communicate between components and trigger actions
 * in response to user input, system events, or other application-specific triggers.
 */
class Event {
public:
  /**
   * @brief Supported event data value types
   * 
   * This uses the common Variant type defined in Types.hh.
   * To add support for additional types, extend the Variant definition.
   */
  using DataValue = Variant;
  
  /**
   * @brief Event constructor
   * 
   * @param type The type of event
   * @param source The source component that triggered the event
   * @throws FabricException if type is empty
   */
  Event(const std::string& type, const std::string& source);
  
  /**
   * @brief Virtual destructor
   */
  virtual ~Event() = default;

  /**
   * @brief Get the event type
   * 
   * @return Event type
   */
  const std::string& getType() const;

  /**
   * @brief Get the event source
   * 
   * @return Event source ID
   */
  const std::string& getSource() const;

  /**
   * @brief Set event data with a key-value pair
   * 
   * @tparam T Type of the data value (must be one of the types in DataValue)
   * @param key Data key
   * @param value Data value
   */
  template <typename T>
  void setData(const std::string& key, const T& value);

  /**
   * @brief Get event data by key
   * 
   * @tparam T Expected type of the data value
   * @param key Data key
   * @return Data value
   * @throws FabricException if data doesn't exist or is wrong type
   */
  template <typename T>
  T getData(const std::string& key) const;
  
  /**
   * @brief Check if data exists with the given key
   * 
   * @param key Data key to check
   * @return true if data exists, false otherwise
   */
  bool hasData(const std::string& key) const;

  /**
   * @brief Check if the event has been handled
   * 
   * @return true if the event has been handled, false otherwise
   */
  bool isHandled() const;

  /**
   * @brief Mark the event as handled
   * 
   * @param handled Whether the event has been handled
   */
  void setHandled(bool handled = true);

  /**
   * @brief Check if the event should propagate to parent components
   * 
   * @return true if the event should propagate, false otherwise
   */
  bool shouldPropagate() const;

  /**
   * @brief Set whether the event should propagate to parent components
   * 
   * @param propagate Whether the event should propagate
   */
  void setPropagate(bool propagate);

private:
  std::string type;
  std::string source;
  mutable std::mutex dataMutex;
  std::unordered_map<std::string, DataValue> data;
  bool handled = false;
  bool propagate = true;
};

/**
 * @brief Event handler function type
 */
using EventHandler = std::function<void(const Event&)>;

/**
 * @brief Event dispatcher class
 * 
 * The EventDispatcher manages event listeners and dispatches events
 * to registered handlers.
 */
class EventDispatcher {
public:
  /**
   * @brief Default constructor
   */
  EventDispatcher() = default;

  /**
   * @brief Add an event listener
   * 
   * @param eventType Event type to listen for
   * @param handler Event handler function
   * @return Handler ID for removal
   * @throws FabricException if eventType is empty or handler is null
   */
  std::string addEventListener(const std::string& eventType, const EventHandler& handler);

  /**
   * @brief Remove an event listener
   * 
   * @param eventType Event type
   * @param handlerId Handler ID to remove
   * @return true if the listener was removed, false otherwise
   */
  bool removeEventListener(const std::string& eventType, const std::string& handlerId);

  /**
   * @brief Dispatch an event
   * 
   * @param event Event to dispatch
   * @return true if the event was handled, false otherwise
   */
  bool dispatchEvent(const Event& event);

private:
  struct HandlerEntry {
    std::string id;
    EventHandler handler;
  };

  mutable std::mutex listenersMutex;
  std::unordered_map<std::string, std::vector<HandlerEntry>> listeners;
};

} // namespace Fabric