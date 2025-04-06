#include "ArgumentParser.hh"
#include "Constants.g.hh"
#include "ErrorHandling.hh"
#include "Logging.hh"
#include "webview/webview.h"
#include <iostream>

void printVersion() {
  std::cout << APP_NAME << " v" << APP_VERSION << std::endl;
}

void printHelp() {
  printVersion();
  std::cout << "Usage: " << APP_EXECUTABLE_NAME << " [options]\n"
            << "Options:\n"
            << "  --help       Show this help message\n"
            << "  --version    Show version information\n";
}

#if defined(_WIN32)
#include <fcntl.h>    // For _O_TEXT
#include <io.h>       // For _open_osfhandle
#include <shellapi.h> // Add this header for CommandLineToArgvW
#include <windows.h>

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
    Fabric::ArgumentParserBuilder parserBuilder;
    Fabric::ArgumentParser parser;
    parserBuilder.addOption("--version", Fabric::TokenType::LiteralString, true)
        .addOption("--help", Fabric::TokenType::LiteralString, true);
    parser = parserBuilder.build();

    bool debug = true;

    parser.parse(argc, argv);

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

    // Check for --help argument
    if (parser.getArgument("--help")) {
      printHelp();
#if defined(_WIN32)
      // Clean up allocated memory before returning
      for (int i = 0; i < argc; i++) {
        delete[] argv[i];
      }
      delete[] argv;
#endif
      return 0;
    }

    // Check for --version argument
    if (parser.getArgument("--version")) {
      printVersion();
#if defined(_WIN32)
      // Clean up allocated memory before returning
      for (int i = 0; i < argc; i++) {
        delete[] argv[i];
      }
      delete[] argv;
#endif
      return 0;
    }

    // Validate required arguments
    if (!parser.isValid()) {
      Fabric::Logger::logError(parser.getErrorMsg());
      printHelp();
#if defined(_WIN32)
      // Clean up allocated memory before returning
      for (int i = 0; i < argc; i++) {
        delete[] argv[i];
      }
      delete[] argv;
#endif
      return 1; // Exit with error code
    }

    webview::webview w(debug, nullptr);
    w.set_title("Fabric Engine Example");
    w.set_size(480, 320, WEBVIEW_HINT_NONE);
    // w.set_html("Thanks for using webview!");
    w.navigate("https://youtube.com");
    w.run();
  } catch (const std::exception &e) {
    std::cerr << "Fatal error: " << e.what() << '\n';
#if defined(_WIN32)
    // Clean up allocated memory in case of exception
    for (int i = 0; i < argc; i++) {
      delete[] argv[i];
    }
    delete[] argv;
#endif
    return 1;
  }

#if defined(_WIN32)
  // Clean up allocated memory before exiting
  for (int i = 0; i < argc; i++) {
    delete[] argv[i];
  }
  delete[] argv;
#endif
  return 0;
}
