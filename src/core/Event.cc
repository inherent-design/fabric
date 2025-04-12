#include "fabric/core/Event.hh"
#include "fabric/utils/ErrorHandling.hh"
#include "fabric/utils/Logging.hh"
#include "fabric/utils/Utils.hh"
#include <type_traits>

namespace Fabric {

Event::Event(const std::string& type, const std::string& source)
    : type(type), source(source) {
  if (type.empty()) {
    throwError("Event type cannot be empty");
  }
}

const std::string& Event::getType() const {
  return type;
}

const std::string& Event::getSource() const {
  return source;
}

bool Event::hasData(const std::string& key) const {
  std::lock_guard<std::mutex> lock(dataMutex);
  return data.find(key) != data.end();
}

template <typename T>
void Event::setData(const std::string& key, const T& value) {
  static_assert(
    std::is_same_v<T, bool> ||
    std::is_same_v<T, int> ||
    std::is_same_v<T, float> ||
    std::is_same_v<T, double> ||
    std::is_same_v<T, std::string>,
    "Data type not supported. Must be one of the types in DataValue."
  );
  
  std::lock_guard<std::mutex> lock(dataMutex);
  data[key] = value;
}

template <typename T>
T Event::getData(const std::string& key) const {
  static_assert(
    std::is_same_v<T, bool> ||
    std::is_same_v<T, int> ||
    std::is_same_v<T, float> ||
    std::is_same_v<T, double> ||
    std::is_same_v<T, std::string>,
    "Data type not supported. Must be one of the types in DataValue."
  );
  
  std::lock_guard<std::mutex> lock(dataMutex);
  
  auto it = data.find(key);
  if (it == data.end()) {
    throwError("Event data key '" + key + "' not found");
  }
  
  try {
    return std::get<T>(it->second);
  } catch (const std::bad_variant_access&) {
    throwError("Event data key '" + key + "' has incorrect type");
    // This is never reached but needed for compilation
    return T();
  }
}

bool Event::isHandled() const {
  return handled;
}

void Event::setHandled(bool handled) {
  this->handled = handled;
}

bool Event::shouldPropagate() const {
  return propagate;
}

void Event::setPropagate(bool propagate) {
  this->propagate = propagate;
}

// Explicit template instantiations for common types
template void Event::setData<int>(const std::string&, const int&);
template void Event::setData<float>(const std::string&, const float&);
template void Event::setData<double>(const std::string&, const double&);
template void Event::setData<bool>(const std::string&, const bool&);
template void Event::setData<std::string>(const std::string&, const std::string&);

template int Event::getData<int>(const std::string&) const;
template float Event::getData<float>(const std::string&) const;
template double Event::getData<double>(const std::string&) const;
template bool Event::getData<bool>(const std::string&) const;
template std::string Event::getData<std::string>(const std::string&) const;

std::string EventDispatcher::addEventListener(const std::string& eventType, const EventHandler& handler) {
  if (eventType.empty()) {
    throwError("Event type cannot be empty");
  }
  
  if (!handler) {
    throwError("Event handler cannot be null");
  }
  
  std::lock_guard<std::mutex> lock(listenersMutex);
  
  HandlerEntry entry;
  entry.id = Utils::generateUniqueId("h_");
  entry.handler = handler;
  
  listeners[eventType].push_back(entry);
  Logger::logDebug("Added event listener for type '" + eventType + "' with ID '" + entry.id + "'");
  
  return entry.id;
}

bool EventDispatcher::removeEventListener(const std::string& eventType, const std::string& handlerId) {
  std::lock_guard<std::mutex> lock(listenersMutex);
  
  auto it = listeners.find(eventType);
  if (it == listeners.end()) {
    return false;
  }
  
  auto& handlers = it->second;
  auto handlerIt = std::find_if(handlers.begin(), handlers.end(),
                               [&handlerId](const HandlerEntry& entry) { return entry.id == handlerId; });
  
  if (handlerIt != handlers.end()) {
    handlers.erase(handlerIt);
    Logger::logDebug("Removed event listener for type '" + eventType + "' with ID '" + handlerId + "'");
    return true;
  }
  
  return false;
}

bool EventDispatcher::dispatchEvent(const Event& event) {
  // We need to make a copy of the handlers to avoid holding the lock during handler execution
  std::vector<HandlerEntry> handlersToInvoke;
  
  {
    std::lock_guard<std::mutex> lock(listenersMutex);
    
    auto it = listeners.find(event.getType());
    if (it == listeners.end()) {
      // No listeners for this event type
      return false;
    }
    
    // Make a copy of the handlers to invoke
    handlersToInvoke = it->second;
  }
  
  bool handled = false;
  
  for (const auto& entry : handlersToInvoke) {
    try {
      entry.handler(event);
      if (event.isHandled()) {
        handled = true;
        break;
      }
    } catch (const std::exception& e) {
      Logger::logError("Exception in event handler: " + std::string(e.what()));
    } catch (...) {
      Logger::logError("Unknown exception in event handler");
    }
  }
  
  return handled;
}

} // namespace Fabric