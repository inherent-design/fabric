#include "fabric/core/Constants.g.hh"
#include "fabric/parser/ArgumentParser.hh"
#include "fabric/ui/WebView.hh"
#include "fabric/utils/Logging.hh"
#include <iostream>

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

#ifdef FABRIC_USE_WEBVIEW
    // Initialize WebView
    Fabric::WebView webview("Fabric", 800, 600, true);
    webview.setHTML("<html><body><h1>Hello from Fabric!</h1><p>Version: " +
                    std::string(Fabric::APP_VERSION) + "</p></body></html>");
    webview.run();
#else
    // WebView is disabled, run in console mode
    std::cout << "WebView is disabled, running in console mode." << std::endl;
    std::cout << "Fabric Engine " << Fabric::APP_VERSION << " initialized successfully." << std::endl;
    std::cout << "Press Enter to exit..." << std::endl;
    std::cin.get();
#endif

    return 0;
  } catch (const std::exception &e) {
    Fabric::Logger::logError("Unhandled exception: " + std::string(e.what()));
    return 1;
  } catch (...) {
    Fabric::Logger::logError("Unknown exception occurred");
    return 1;
  }
}
