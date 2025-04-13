# Event Handling and Propagation

[← Back to Examples Index](../EXAMPLES.md)

This example demonstrates how to work with the event system including creating, dispatching, and handling events:

```cpp
#include "fabric/core/Component.hh"
#include "fabric/core/Event.hh"
#include "fabric/utils/Logging.hh"

using namespace Fabric;

// Define a form component that contains a button
class Form : public Component {
public:
  Form(const std::string& id) : Component(id), dispatcher() {
    // Create and add a button child component
    auto button = std::make_shared<Button>("submit-button");
    button->setProperty<std::string>("label", "Submit");
    addChild(button);
    
    // Create and add an input field child component
    auto input = std::make_shared<Input>("email-input");
    input->setProperty<std::string>("placeholder", "Enter your email");
    addChild(input);
    
    // Register event listeners
    dispatcher.addEventListener("submit", [this](const Event& event) {
      handleSubmit(event);
    });
    
    dispatcher.addEventListener("input", [this](const Event& event) {
      handleInput(event);
    });
  }
  
  void initialize() override {
    // Initialize the form and all its children
    for (const auto& child : getChildren()) {
      child->initialize();
    }
  }
  
  std::string render() override {
    std::string formHTML = "<form id='" + getId() + "'>\n";
    
    // Render all child components
    for (const auto& child : getChildren()) {
      formHTML += "  " + child->render() + "\n";
    }
    
    formHTML += "</form>";
    return formHTML;
  }
  
  void update(float deltaTime) override {
    // Update all child components
    for (const auto& child : getChildren()) {
      child->update(deltaTime);
    }
  }
  
  void cleanup() override {
    // Clean up all child components
    for (const auto& child : getChildren()) {
      child->cleanup();
    }
  }
  
  // Process events from child components
  void processEvent(const Event& event) {
    // Log the event
    Logger::logInfo("Form received event: " + event.getType() + 
                    " from " + event.getSource());
    
    // Check if it's a button click
    if (event.getType() == "click" && 
        event.getSource() == "submit-button") {
      
      // Create and dispatch a submit event
      Event submitEvent("submit", getId());
      
      // Add form data to the event
      auto emailInput = getChild("email-input");
      if (emailInput) {
        submitEvent.setData<std::string>("email", 
            emailInput->getProperty<std::string>("value"));
      }
      
      // Mark the original event as handled
      event.setHandled(true);
      
      // Dispatch the submit event
      dispatcher.dispatchEvent(submitEvent);
    }
    
    // If the event wasn't handled and should propagate,
    // it will bubble up to the parent component
  }
  
private:
  EventDispatcher dispatcher;
  
  void handleSubmit(const Event& event) {
    if (event.hasData("email")) {
      std::string email = event.getData<std::string>("email");
      Logger::logInfo("Form submitted with email: " + email);
      
      // Example: Validate email
      if (email.find('@') != std::string::npos) {
        // Valid email, proceed with submission
        setProperty<bool>("isSubmitting", true);
      } else {
        // Invalid email, show error
        auto emailInput = getChild("email-input");
        if (emailInput) {
          emailInput->setProperty<std::string>("errorMessage", 
                                             "Please enter a valid email");
          emailInput->setProperty<bool>("hasError", true);
        }
      }
    }
  }
  
  void handleInput(const Event& event) {
    // Clear any previous errors when user types
    if (event.getSource() == "email-input") {
      auto emailInput = getChild("email-input");
      if (emailInput && emailInput->getProperty<bool>("hasError")) {
        emailInput->setProperty<bool>("hasError", false);
        emailInput->setProperty<std::string>("errorMessage", "");
      }
    }
  }
};

// Simple Input component for demonstration
class Input : public Component {
public:
  Input(const std::string& id) : Component(id) {
    setProperty<std::string>("value", "");
    setProperty<std::string>("placeholder", "");
    setProperty<bool>("hasError", false);
    setProperty<std::string>("errorMessage", "");
  }
  
  // Other methods omitted for brevity
  void initialize() override {}
  void update(float deltaTime) override {}
  void cleanup() override {}
  
  std::string render() override {
    std::string html = "<div class='input-wrapper'>";
    html += "<input type='text' id='" + getId() + "' ";
    html += "value='" + getProperty<std::string>("value") + "' ";
    html += "placeholder='" + getProperty<std::string>("placeholder") + "' ";
    
    // Add error styling if has error
    if (getProperty<bool>("hasError")) {
      html += "class='error' ";
    }
    
    html += "/>";
    
    // Show error message if any
    if (getProperty<bool>("hasError")) {
      html += "<p class='error-message'>" + 
               getProperty<std::string>("errorMessage") + 
              "</p>";
    }
    
    html += "</div>";
    return html;
  }
};

// Example of event flow
void eventHandlingExample() {
  // Create form component
  auto form = std::make_shared<Form>("contact-form");
  form->initialize();
  
  // Render the form
  std::string html = form->render();
  std::cout << "Form HTML: " << html << std::endl;
  
  // Simulate user input
  auto emailInput = form->getChild("email-input");
  emailInput->setProperty<std::string>("value", "user@example.com");
  
  // Create an input event
  Event inputEvent("input", "email-input");
  inputEvent.setData<std::string>("value", "user@example.com");
  
  // Process the input event
  form->processEvent(inputEvent);
  
  // Simulate button click
  Event clickEvent("click", "submit-button");
  form->processEvent(clickEvent);
  
  // Check if form is now submitting
  std::cout << "Form is submitting: " << 
    (form->getProperty<bool>("isSubmitting") ? "Yes" : "No") << std::endl;
}
```

[← Back to Examples Index](../EXAMPLES.md)