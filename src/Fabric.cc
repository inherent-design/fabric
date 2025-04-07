#include "ArgumentParser.hh"
#include "Constants.g.hh"
#include "ErrorHandling.hh"
#include "Logging.hh"
#include "Utils.hh"
#include "WebView.hh"
#include <iostream>

/**
 * @brief Print version information to stdout
 */
void printVersion() {
  std::cout << APP_NAME << " v" << APP_VERSION << std::endl;
}

/**
 * @brief Print help message to stdout
 */
void printHelp() {
  printVersion();
  std::cout << "Usage: " << APP_EXECUTABLE_NAME << " [options]\n"
            << "Options:\n"
            << "  --help       Show this help message\n"
            << "  --version    Show version information\n";
}

#if defined(_WIN32)
/**
 * @brief Helper function to clean up Windows command line arguments
 * @param argc Number of arguments
 * @param argv Array of argument strings
 */
void cleanupWindowsArgs(int argc, char **argv) {
  for (int i = 0; i < argc; i++) {
    delete[] argv[i];
  }
  delete[] argv;
}
#endif

#if defined(_WIN32)
#include <fcntl.h>    // For _O_TEXT
#include <io.h>       // For _open_osfhandle
#include <shellapi.h> // Add this header for CommandLineToArgvW
#include <windows.h>
#endif

/**
 * @brief Initialize the argument parser
 * @return Configured ArgumentParser instance
 */
Fabric::ArgumentParser initArgumentParser() {
  Fabric::ArgumentParserBuilder parserBuilder;
  parserBuilder.addOption("--version", Fabric::TokenType::LiteralString, true)
      .addOption("--help", Fabric::TokenType::LiteralString, true);
  return parserBuilder.build();
}

/**
 * @brief Setup console output on Windows in debug mode
 * @param debug Whether debug mode is enabled
 */
void setupConsole(bool debug) {
#if defined(_WIN32)
  // Only allocate a console for output on Windows when in debug mode
  if (debug) {
    AllocConsole();

    // Redirect stdout to the console
    FILE *pConsole;
    freopen_s(&pConsole, "CONOUT$", "w", stdout);

    // Redirect stderr to the console
    freopen_s(&pConsole, "CONOUT$", "w", stderr);
  }
#endif
}

/**
 * @brief Handle command line arguments
 * @param parser The argument parser
 * @param argc Number of arguments
 * @param argv Array of argument strings
 * @return Exit code (0 for success, non-zero for error)
 */
int handleCommandLineArgs(const Fabric::ArgumentParser &parser, int argc,
                          char *argv[]) {
  // Check for --help argument
  if (parser.getArgument("--help")) {
    printHelp();
#if defined(_WIN32)
    cleanupWindowsArgs(argc, argv);
#endif
    return 0;
  }

  // Check for --version argument
  if (parser.getArgument("--version")) {
    printVersion();
#if defined(_WIN32)
    cleanupWindowsArgs(argc, argv);
#endif
    return 0;
  }

  // Validate required arguments
  if (!parser.isValid()) {
    Fabric::Logger::logError(parser.getErrorMsg());
    printHelp();
#if defined(_WIN32)
    cleanupWindowsArgs(argc, argv);
#endif
    return 1; // Exit with error code
  }

  return -1; // Continue execution
}

/**
 * @brief Initialize and run the WebView
 * @param debug Whether debug mode is enabled
 */
void runWebView(bool debug) {
  // Create and configure the WebView
  Fabric::WebView webView(debug);
  webView.setTitle("Fabric Engine");
  webView.setSize(480, 320, WEBVIEW_HINT_NONE);
  // webView.setHtml("Thanks for using Fabric Engine!");
  webView.navigate("https://youtube.com");
  webView.run();
}

#if defined(_WIN32)
int WINAPI WinMain(HINSTANCE /*hInst*/, HINSTANCE /*hPrevInst*/,
                   PSTR /*lpCmdLine*/, int /*nCmdShow*/) {

  // Get command line arguments on Windows
  int argc = 0;
  LPWSTR *argvW = CommandLineToArgvW(GetCommandLineW(), &argc);

  // Convert wide strings to regular strings
  char **argv = new char *[argc];
  for (int i = 0; i < argc; i++) {
    int size = WideCharToMultiByte(CP_UTF8, 0, argvW[i], -1, nullptr, 0,
                                   nullptr, nullptr);
    argv[i] = new char[size];
    WideCharToMultiByte(CP_UTF8, 0, argvW[i], -1, argv[i], size, nullptr,
                        nullptr);
  }
  LocalFree(argvW);

#else
int main(int argc, char *argv[]) {
#endif
  try {
    // Initialize argument parser
    Fabric::ArgumentParser parser = initArgumentParser();

    // Set debug mode
    bool debug = true;

    // Parse command line arguments
    parser.parse(argc, argv);

    // Setup console output (Windows-specific)
    setupConsole(debug);

    // Handle command line arguments
    int result = handleCommandLineArgs(parser, argc, argv);
    if (result >= 0) {
      return result;
    }

    // Run the WebView
    runWebView(debug);
  } catch (const std::exception &e) {
    std::cerr << "Fatal error: " << e.what() << '\n';
#if defined(_WIN32)
    cleanupWindowsArgs(argc, argv);
#endif
    return 1;
  }

#if defined(_WIN32)
  cleanupWindowsArgs(argc, argv);
#endif
  return 0;
}
