#include "fabric/core/Constants.g.hh"
#include "fabric/core/Command.hh"
#include "fabric/core/Reactive.hh"
#include "fabric/core/Resource.hh"
#include "fabric/core/Spatial.hh"
#include "fabric/core/Temporal.hh"
#include "fabric/core/ResourceHub.hh"
#include "fabric/parser/ArgumentParser.hh"
#include "fabric/ui/WebView.hh"
#include "fabric/utils/Logging.hh"
#include "fabric/utils/CoordinatedGraph.hh"
#include <iostream>
#include <SDL3/SDL.h>
#include <chrono>
#include <thread>

// Demo class that combines features from all the Quantum Fluctuation systems
class FabricDemo {
private:
    // Command system
    Fabric::CommandManager commandManager;
    
    // Spatial system
    fabric::core::Scene scene;
    
    // Reactive system
    Fabric::Observable<int> counter{0};
    Fabric::ComputedValue<std::string> counterText;
    
    // Temporal system
    fabric::core::Timeline& timeline;
    fabric::core::TimeRegion* mainRegion;
    
    // Resource system
    Fabric::ResourceHub& resourceHub;
    
    // WebView for UI
    Fabric::WebView webview;
    
    // SDL
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    bool running = true;

public:
    FabricDemo() : 
        counterText([this]() { 
            return "Counter: " + std::to_string(counter.get()); 
        }),
        timeline(fabric::core::Timeline::instance()),
        resourceHub(Fabric::ResourceHub::instance()),
        webview("Fabric Quantum Demo", 800, 600, false) {
        
        // Set up WebView with basic HTML
        setupWebView();
        
        // Create time region for main simulation
        mainRegion = timeline.createRegion(1.0);
        
        // Set up reactive connections
        counter.observe([this](const int& oldVal, const int& newVal) {
            Fabric::Logger::logInfo("Counter changed: " + std::to_string(oldVal) + " -> " + std::to_string(newVal));
            updateWebView();
        });
        
        // Set up SDL
        if (SDL_Init(SDL_INIT_VIDEO) != 0) {
            throw std::runtime_error("SDL initialization failed: " + std::string(SDL_GetError()));
        }
        
        // Create window for rendering demo (small window alongside webview)
        SDL_WindowFlags windowFlags = static_cast<SDL_WindowFlags>(0);
        window = SDL_CreateWindow("Fabric Spatial Demo", 400, 300, windowFlags);
        if (!window) {
            SDL_Quit();
            throw std::runtime_error("SDL window creation failed: " + std::string(SDL_GetError()));
        }
        
        renderer = SDL_CreateRenderer(window, nullptr);
        if (!renderer) {
            SDL_DestroyWindow(window);
            SDL_Quit();
            throw std::runtime_error("SDL renderer creation failed: " + std::string(SDL_GetError()));
        }
        
        // Create basic scene structure
        setupScene();
    }
    
    ~FabricDemo() {
        if (renderer) {
            SDL_DestroyRenderer(renderer);
        }
        if (window) {
            SDL_DestroyWindow(window);
        }
        SDL_Quit();
    }
    
    void setupWebView() {
        std::string html = R"(
            <!DOCTYPE html>
            <html>
            <head>
                <title>Fabric Quantum Demo</title>
                <style>
                    body { font-family: Arial, sans-serif; margin: 0; padding: 20px; }
                    h1 { color: #333; }
                    button { padding: 10px; margin: 5px; cursor: pointer; }
                    #counter { font-size: 24px; margin: 20px 0; }
                </style>
            </head>
            <body>
                <h1>Fabric Quantum Fluctuation Demo</h1>
                <div id="counter">Counter: 0</div>
                <button id="incrementBtn">Increment</button>
                <button id="decrementBtn">Decrement</button>
                <button id="resetBtn">Reset</button>
                <div>
                    <h3>Time Controls</h3>
                    <button id="slowTimeBtn">Slow Time (0.5x)</button>
                    <button id="normalTimeBtn">Normal Time (1.0x)</button>
                    <button id="fastTimeBtn">Fast Time (2.0x)</button>
                </div>
                <script>
                    document.getElementById('incrementBtn').onclick = function() {
                        window.external.invoke('increment');
                    };
                    document.getElementById('decrementBtn').onclick = function() {
                        window.external.invoke('decrement');
                    };
                    document.getElementById('resetBtn').onclick = function() {
                        window.external.invoke('reset');
                    };
                    document.getElementById('slowTimeBtn').onclick = function() {
                        window.external.invoke('setTimeScale:0.5');
                    };
                    document.getElementById('normalTimeBtn').onclick = function() {
                        window.external.invoke('setTimeScale:1.0');
                    };
                    document.getElementById('fastTimeBtn').onclick = function() {
                        window.external.invoke('setTimeScale:2.0');
                    };
                </script>
            </body>
            </html>
        )";
        
        webview.setHTML(html);
        
        // Set up callback handler for JavaScript bridge
        webview.bind("invoke", [this](const std::string& message) -> std::string {
            if (message == "increment") {
                auto incrementCmd = Fabric::makeCommand<int>(
                    [this](int& state) { counter.set(counter.get() + 1); },
                    0,
                    "Increment Counter"
                );
                commandManager.execute(std::move(incrementCmd));
            } 
            else if (message == "decrement") {
                auto decrementCmd = Fabric::makeCommand<int>(
                    [this](int& state) { counter.set(counter.get() - 1); },
                    0,
                    "Decrement Counter"
                );
                commandManager.execute(std::move(decrementCmd));
            }
            else if (message == "reset") {
                auto resetCmd = Fabric::makeCommand<int>(
                    [this](int& state) { counter.set(0); },
                    0,
                    "Reset Counter"
                );
                commandManager.execute(std::move(resetCmd));
            }
            else if (message.find("setTimeScale:") == 0) {
                std::string scaleStr = message.substr(13);
                try {
                    double scale = std::stod(scaleStr);
                    mainRegion->setTimeScale(scale);
                    Fabric::Logger::logInfo("Set time scale to " + scaleStr);
                } catch (const std::exception& e) {
                    Fabric::Logger::logError("Invalid time scale: " + scaleStr);
                }
            }
            return "ok";
        });
    }
    
    void updateWebView() {
        std::string js = "document.getElementById('counter').textContent = '" + counterText.get() + "';";
        webview.eval(js);
    }
    
    void setupScene() {
        // Create a basic scene with a few nodes
        auto root = scene.getRoot();
        
        // Add some child nodes
        auto node1 = root->createChild("node1");
        auto node2 = root->createChild("node2");
        auto node3 = node1->createChild("node1_child");
        
        // Set up transforms
        node1->setPosition(fabric::core::Vector3<float, fabric::core::Space::World>(1.0f, 0.0f, 0.0f));
        
        node2->setPosition(fabric::core::Vector3<float, fabric::core::Space::World>(-1.0f, 0.0f, 0.0f));
        
        node3->setPosition(fabric::core::Vector3<float, fabric::core::Space::World>(0.0f, 1.0f, 0.0f));
    }
    
    void renderScene() {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        
        // Render the scene nodes as simple shapes
        auto root = scene.getRoot();
        renderNode(root, fabric::core::Transform<float>());
        
        SDL_RenderPresent(renderer);
    }
    
    void renderNode(fabric::core::SceneNode* node, const fabric::core::Transform<float>& parentTransform) {
        if (!node) return;
        
        // Combine transforms
        auto combinedTransform = parentTransform.combine(node->getLocalTransform());
        
        // Render this node as a rectangle
        fabric::core::Vector3<float, fabric::core::Space::World> pos = combinedTransform.getPosition();
        
        // Convert to screen coordinates (center of screen is origin)
        int screenX = 200 + static_cast<int>(pos.x * 50);
        int screenY = 150 + static_cast<int>(pos.y * 50);
        
        // Draw a rectangle
        SDL_Rect rect = {screenX - 10, screenY - 10, 20, 20};
        
        // Use different colors for different nodes
        if (node->getName() == "root") {
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        } else if (node->getName().find("node1") != std::string::npos) {
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        } else if (node->getName().find("node2") != std::string::npos) {
            SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
        } else {
            SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
        }
        
        SDL_FRect frect = {(float)rect.x, (float)rect.y, (float)rect.w, (float)rect.h};
        SDL_RenderRect(renderer, &frect);
        
        // Recursively render children
        for (size_t i = 0; i < node->getChildCount(); ++i) {
            renderNode(node->getChild(i), combinedTransform);
        }
    }
    
    void run() {
        // Launch WebView in a separate thread
        std::thread webviewThread([this]() {
            this->webview.run();
            this->running = false;
        });
        
        // Main SDL loop for updating and rendering
        auto lastFrameTime = std::chrono::high_resolution_clock::now();
        while (running) {
            // Process SDL events
            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_EVENT_QUIT) {
                    running = false;
                }
            }
            
            // Calculate delta time
            auto currentTime = std::chrono::high_resolution_clock::now();
            float deltaTime = std::chrono::duration<float>(currentTime - lastFrameTime).count();
            lastFrameTime = currentTime;
            
            // Update timeline
            timeline.update(deltaTime);
            
            // Update scene
            scene.update(deltaTime);
            
            // Render scene
            renderScene();
            
            // Prevent excessive CPU usage
            SDL_Delay(16); // ~60 FPS
        }
        
        if (webviewThread.joinable()) {
            webviewThread.join();
        }
    }
};

int main(int argc, char *argv[]) {
  try {
    // Initialize logger
    Fabric::Logger::initialize();
    Fabric::Logger::logInfo("Starting " + std::string(Fabric::APP_NAME) + " " +
                            std::string(Fabric::APP_VERSION));

    // Parse command line arguments
    Fabric::ArgumentParser argParser;
    argParser.addArgument("--version", "Display version information");
    argParser.addArgument("--help", "Display help information");
    argParser.parse(argc, argv);

    // Check for version flag
    if (argParser.hasArgument("--version")) {
      std::cout << Fabric::APP_NAME << " version " << Fabric::APP_VERSION
                << std::endl;
      return 0;
    }

    // Check for help flag
    if (argParser.hasArgument("--help")) {
      std::cout << "Usage: " << Fabric::APP_EXECUTABLE_NAME << " [options]"
                << std::endl;
      std::cout << "Options:" << std::endl;
      std::cout << "  --version    Display version information" << std::endl;
      std::cout << "  --help       Display this help message" << std::endl;
      return 0;
    }

    // Create and run the FabricDemo
    FabricDemo demo;
    demo.run();

    return 0;
  } catch (const std::exception &e) {
    Fabric::Logger::logError("Unhandled exception: " + std::string(e.what()));
    return 1;
  } catch (...) {
    Fabric::Logger::logError("Unknown exception occurred");
    return 1;
  }
}
