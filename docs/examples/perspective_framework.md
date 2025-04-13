# Multi-Scale Perspective Framework

[← Back to Examples Index](../EXAMPLES.md)

The following example shows a basic implementation of the perspective-fluid architecture using our Component-based system:

```cpp
#include "fabric/core/Component.hh"
#include "fabric/core/Event.hh"
#include "fabric/utils/Logging.hh"
#include <memory>
#include <cmath>

using namespace Fabric;

// Forward declarations
class ScalePerspective;
class Quantum;

// Scale definitions
enum class Scale {
  Quantum,  // Subatomic scale
  Micro,    // Microscopic scale
  Meso,     // Human-scale
  Macro,    // Planetary scale
  Cosmic    // Universe scale
};

// Observer class that maintains a specific perspective
class Observer : public Component {
public:
  Observer(const std::string& id) : Component(id) {
    setProperty<Scale>("scale", Scale::Meso);
    setProperty<float>("perceptionRadius", 100.0f);
  }
  
  void initialize() override {
    Logger::logInfo("Observer " + getId() + " initialized");
  }
  
  std::string render() override {
    return "<div class='observer' id='" + getId() + "'></div>";
  }
  
  void update(float deltaTime) override {
    // Update perspective based on focus point
    if (hasProperty("focusTarget") && hasProperty("currentPerspective")) {
      auto target = getProperty<std::shared_ptr<Component>>("focusTarget");
      auto perspective = getProperty<std::shared_ptr<ScalePerspective>>("currentPerspective");
      
      if (target && perspective) {
        perspective->updateObserver(this, target);
      }
    }
  }
  
  void cleanup() override {
    Logger::logInfo("Observer " + getId() + " cleaned up");
  }
  
  // Set the scale at which this observer perceives reality
  void setObservationScale(Scale scale) {
    setProperty<Scale>("scale", scale);
    
    // Adjust perception radius based on scale
    float radius = 0.0f;
    switch (scale) {
      case Scale::Quantum:
        radius = 1e-10f;
        break;
      case Scale::Micro:
        radius = 1e-3f;
        break;
      case Scale::Meso:
        radius = 100.0f;
        break;
      case Scale::Macro:
        radius = 1e6f;
        break;
      case Scale::Cosmic:
        radius = 1e12f;
        break;
    }
    
    setProperty<float>("perceptionRadius", radius);
  }
  
  // Focus on a specific quantum object
  void focusOn(std::shared_ptr<Component> target) {
    setProperty<std::shared_ptr<Component>>("focusTarget", target);
    
    // Create appropriate perspective for the target
    auto perspective = createPerspectiveForTarget(target);
    setProperty<std::shared_ptr<ScalePerspective>>("currentPerspective", perspective);
    
    // Update immediately
    update(0.0f);
  }
  
  // Perceive a quantum within the current perspective
  std::shared_ptr<Component> perceive(std::shared_ptr<Component> quantum) {
    if (hasProperty("currentPerspective")) {
      auto perspective = getProperty<std::shared_ptr<ScalePerspective>>("currentPerspective");
      if (perspective) {
        return perspective->renderQuantumForObserver(this, quantum);
      }
    }
    
    return nullptr;
  }
  
private:
  // Create appropriate perspective for a target
  std::shared_ptr<ScalePerspective> createPerspectiveForTarget(std::shared_ptr<Component> target) {
    // Get the target's scale
    Scale targetScale = Scale::Meso;
    if (target->hasProperty("inherentScale")) {
      targetScale = target->getProperty<Scale>("inherentScale");
    }
    
    // Create perspective based on target scale
    auto perspective = std::make_shared<ScalePerspective>("perspective-" + target->getId());
    perspective->setProperty<Scale>("scale", targetScale);
    
    return perspective;
  }
};

// Manages how objects are perceived at different scales
class ScalePerspective : public Component {
public:
  ScalePerspective(const std::string& id) : Component(id) {
    setProperty<Scale>("scale", Scale::Meso);
  }
  
  void initialize() override {}
  void update(float deltaTime) override {}
  void cleanup() override {}
  std::string render() override { return ""; }
  
  // Update observer state based on target
  void updateObserver(Observer* observer, std::shared_ptr<Component> target) {
    // Calculate distance factor
    float distanceFactor = 1.0f;
    if (observer->hasProperty("position") && target->hasProperty("position")) {
      // This is simplified - in reality would use 3D vectors
      float observerPos = observer->getProperty<float>("position");
      float targetPos = target->getProperty<float>("position");
      float distance = std::abs(targetPos - observerPos);
      
      // Distance affects perceived scale
      distanceFactor = 1.0f / (1.0f + distance * 0.01f);
    }
    
    // Get scales
    Scale observerScale = observer->getProperty<Scale>("scale");
    Scale targetScale = target->getProperty<Scale>("inherentScale");
    Scale perspectiveScale = getProperty<Scale>("scale");
    
    // Determine which properties are visible to the observer
    determineVisibleProperties(observer, target, observerScale, 
                              targetScale, perspectiveScale, distanceFactor);
  }
  
  // Render a quantum for a specific observer
  std::shared_ptr<Component> renderQuantumForObserver(Observer* observer, 
                                                     std::shared_ptr<Component> quantum) {
    // Create a view of the quantum that's appropriate for the observer
    auto observerScale = observer->getProperty<Scale>("scale");
    auto quantumScale = quantum->getProperty<Scale>("inherentScale");
    
    // Create a view component
    auto view = std::make_shared<Component>("view-" + quantum->getId());
    
    // Transfer basic properties
    view->setProperty<std::string>("name", quantum->getProperty<std::string>("name"));
    view->setProperty<std::string>("type", quantum->getProperty<std::string>("type"));
    
    // Scale-dependent rendering
    switch (observerScale) {
      case Scale::Quantum:
        // At quantum scale, show wave functions, uncertainty, etc.
        if (quantum->hasProperty("waveFunction")) {
          view->setProperty<std::string>("waveFunction", 
                                        quantum->getProperty<std::string>("waveFunction"));
        }
        if (quantum->hasProperty("spin")) {
          view->setProperty<float>("spin", quantum->getProperty<float>("spin"));
        }
        break;
        
      case Scale::Micro:
        // At microscopic scale, show molecular structures, etc.
        if (quantum->hasProperty("molecularStructure")) {
          view->setProperty<std::string>("structure", 
                                       quantum->getProperty<std::string>("molecularStructure"));
        }
        break;
        
      case Scale::Meso:
        // At human scale, show typical observable properties
        if (quantum->hasProperty("color")) {
          view->setProperty<std::string>("color", quantum->getProperty<std::string>("color"));
        }
        if (quantum->hasProperty("shape")) {
          view->setProperty<std::string>("shape", quantum->getProperty<std::string>("shape"));
        }
        if (quantum->hasProperty("texture")) {
          view->setProperty<std::string>("texture", quantum->getProperty<std::string>("texture"));
        }
        break;
        
      case Scale::Macro:
        // At macro scale, show aggregate properties
        if (quantum->hasProperty("mass")) {
          view->setProperty<float>("mass", quantum->getProperty<float>("mass"));
        }
        if (quantum->hasProperty("volume")) {
          view->setProperty<float>("volume", quantum->getProperty<float>("volume"));
        }
        break;
        
      case Scale::Cosmic:
        // At cosmic scale, show only the most significant properties
        if (quantum->hasProperty("gravitationalField")) {
          view->setProperty<float>("gravity", 
                                 quantum->getProperty<float>("gravitationalField"));
        }
        break;
    }
    
    return view;
  }
  
private:
  // Determine which properties of a quantum are visible to an observer
  void determineVisibleProperties(Observer* observer, std::shared_ptr<Component> target,
                                 Scale observerScale, Scale targetScale, Scale perspectiveScale,
                                 float distanceFactor) {
    // Scale difference factor
    int scaleDifference = std::abs(static_cast<int>(observerScale) - 
                                  static_cast<int>(targetScale));
    
    // Visibility threshold
    float visibilityThreshold = 0.5f;
    float visibility = distanceFactor / (1.0f + scaleDifference);
    
    // Update observer's perception
    observer->setProperty<float>("currentVisibility", visibility);
    observer->setProperty<bool>("canPerceiveTarget", visibility > visibilityThreshold);
    
    // Log the observation
    std::string scaleStr;
    switch (observerScale) {
      case Scale::Quantum: scaleStr = "Quantum"; break;
      case Scale::Micro: scaleStr = "Micro"; break;
      case Scale::Meso: scaleStr = "Meso"; break;
      case Scale::Macro: scaleStr = "Macro"; break;
      case Scale::Cosmic: scaleStr = "Cosmic"; break;
    }
    
    Logger::logInfo("Observer at " + scaleStr + " scale observing target with " +
                    std::to_string(visibility) + " visibility");
  }
};

// A quantum object that can be observed across scales
class Quantum : public Component {
public:
  Quantum(const std::string& id) : Component(id) {
    // Basic properties
    setProperty<std::string>("name", id);
    setProperty<std::string>("type", "GenericQuantum");
    setProperty<Scale>("inherentScale", Scale::Meso);
    setProperty<float>("position", 0.0f);
    
    // Quantum scale properties (typically hidden at macro scale)
    setProperty<std::string>("waveFunction", "Psi(x) = A*exp(i*k*x - i*omega*t)");
    setProperty<float>("spin", 0.5f);
    setProperty<bool>("entangled", false);
    
    // Micro scale properties
    setProperty<std::string>("molecularStructure", "Tetrahedral");
    setProperty<int>("atomicNumber", 6);  // Carbon
    
    // Meso scale properties (visible to human observers)
    setProperty<std::string>("color", "gray");
    setProperty<std::string>("shape", "cube");
    setProperty<std::string>("texture", "smooth");
    
    // Macro scale properties
    setProperty<float>("mass", 1.0f);
    setProperty<float>("volume", 1.0f);
    setProperty<float>("temperature", 293.15f);  // Room temperature in K
    
    // Cosmic scale properties
    setProperty<float>("gravitationalField", 9.8f);
  }
  
  void initialize() override {
    Logger::logInfo("Quantum " + getId() + " initialized");
  }
  
  void update(float deltaTime) override {
    // Update quantum state over time
    if (hasProperty("entangled") && getProperty<bool>("entangled")) {
      // Update linked quantum properties
    }
  }
  
  std::string render() override {
    // Base rendering is minimal - actual rendering depends on observer
    return "<div class='quantum' id='" + getId() + "'></div>";
  }
  
  void cleanup() override {
    Logger::logInfo("Quantum " + getId() + " cleaned up");
  }
  
  // Create different types of quantum objects
  static std::shared_ptr<Quantum> createAtom(const std::string& id, int atomicNumber) {
    auto atom = std::make_shared<Quantum>(id);
    atom->setProperty<std::string>("type", "Atom");
    atom->setProperty<Scale>("inherentScale", Scale::Micro);
    atom->setProperty<int>("atomicNumber", atomicNumber);
    
    // Set other properties based on atomic number
    switch (atomicNumber) {
      case 1:  // Hydrogen
        atom->setProperty<std::string>("name", "Hydrogen");
        atom->setProperty<float>("mass", 1.008f);
        break;
      case 6:  // Carbon
        atom->setProperty<std::string>("name", "Carbon");
        atom->setProperty<float>("mass", 12.011f);
        break;
      case 8:  // Oxygen
        atom->setProperty<std::string>("name", "Oxygen");
        atom->setProperty<float>("mass", 15.999f);
        break;
      // More elements could be added
    }
    
    return atom;
  }
  
  static std::shared_ptr<Quantum> createStar(const std::string& id, float mass) {
    auto star = std::make_shared<Quantum>(id);
    star->setProperty<std::string>("type", "Star");
    star->setProperty<Scale>("inherentScale", Scale::Cosmic);
    star->setProperty<float>("mass", mass);
    star->setProperty<float>("temperature", 5778.0f);  // Sun-like by default
    star->setProperty<std::string>("color", "yellow");
    
    // More properties like radius, luminosity, etc.
    return star;
  }
};

// Example using the multi-scale perspective framework
void multiScalePerspectiveExample() {
  // Create some quantum objects at different scales
  auto electron = std::make_shared<Quantum>("electron-1");
  electron->setProperty<Scale>("inherentScale", Scale::Quantum);
  electron->setProperty<float>("spin", 0.5f);
  electron->setProperty<float>("position", 0.0f);
  
  auto carbon = Quantum::createAtom("carbon-1", 6);
  carbon->setProperty<float>("position", 10.0f);
  
  auto star = Quantum::createStar("sun", 1.989e30f);
  star->setProperty<float>("position", 1.0e8f);
  
  // Create observers at different scales
  auto quantumObserver = std::make_shared<Observer>("quantum-observer");
  quantumObserver->setObservationScale(Scale::Quantum);
  quantumObserver->setProperty<float>("position", 0.1f);
  
  auto mesoObserver = std::make_shared<Observer>("meso-observer");
  mesoObserver->setObservationScale(Scale::Meso);
  mesoObserver->setProperty<float>("position", 5.0f);
  
  auto cosmicObserver = std::make_shared<Observer>("cosmic-observer");
  cosmicObserver->setObservationScale(Scale::Cosmic);
  cosmicObserver->setProperty<float>("position", 1.0e9f);
  
  // Initialize all components
  electron->initialize();
  carbon->initialize();
  star->initialize();
  
  quantumObserver->initialize();
  mesoObserver->initialize();
  cosmicObserver->initialize();
  
  // Observe the same objects from different perspectives
  std::cout << "Quantum Observer focusing on electron:" << std::endl;
  quantumObserver->focusOn(electron);
  auto electronView = quantumObserver->perceive(electron);
  if (electronView) {
    std::cout << "Electron spin: " << 
                electronView->getProperty<float>("spin") << std::endl;
  }
  
  std::cout << "\nMeso Observer focusing on carbon atom:" << std::endl;
  mesoObserver->focusOn(carbon);
  auto carbonView = mesoObserver->perceive(carbon);
  if (carbonView) {
    std::cout << "Carbon atom appears as: " << 
                carbonView->getProperty<std::string>("shape") << " shaped, " <<
                carbonView->getProperty<std::string>("color") << " colored" << 
                std::endl;
  }
  
  std::cout << "\nCosmic Observer focusing on star:" << std::endl;
  cosmicObserver->focusOn(star);
  auto starView = cosmicObserver->perceive(star);
  if (starView) {
    std::cout << "Star gravitational field: " << 
                starView->getProperty<float>("gravity") << std::endl;
  }
  
  // An observer at the wrong scale may not perceive certain objects
  std::cout << "\nCosmic Observer trying to perceive electron:" << std::endl;
  auto electronViewByCosmicObserver = cosmicObserver->perceive(electron);
  if (electronViewByCosmicObserver) {
    std::cout << "Properties perceived: " << 
                electronViewByCosmicObserver->render() << std::endl;
  } else {
    std::cout << "Electron is imperceptible at cosmic scale" << std::endl;
  }
  
  // Shift an observer's scale to see different aspects
  std::cout << "\nShifting Meso Observer to Quantum scale:" << std::endl;
  mesoObserver->setObservationScale(Scale::Quantum);
  mesoObserver->focusOn(electron);
  auto newElectronView = mesoObserver->perceive(electron);
  if (newElectronView) {
    std::cout << "Now perceives electron wave function: " << 
                newElectronView->getProperty<std::string>("waveFunction") << 
                std::endl;
  }
}
```

[← Back to Examples Index](../EXAMPLES.md)