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
#include <windows.h>
int WINAPI WinMain(HINSTANCE /*hInst*/, HINSTANCE /*hPrevInst*/,
                   LPSTR /*lpCmdLine*/, int /*nCmdShow*/) {
#else
int main(int argc, char *argv[]) {
#endif
  try {
    ArgumentParserBuilder parserBuilder;
    ArgumentParser parser;
    parserBuilder.addOption("--version", TokenType::LiteralString, true)
        .addOption("--help", TokenType::LiteralString, true);
    parser = parserBuilder.build();

#if defined(_WIN32)
    bool debug = false;
#else
    bool debug = true;
#endif

    parser.parse(argc, argv);

    // Check for --help argument
    if (parser.getArgument("--help")) {
      printHelp();
      return 0;
    }

    // Check for --version argument
    if (parser.getArgument("--version")) {
      printVersion();
      return 0;
    }

    // Validate required arguments
    if (!parser.isValid()) {
      Logger::logError(parser.getErrorMsg());
      printHelp();
      return 1; // Exit with error code
    }

    webview::webview w(debug, nullptr);
    w.set_title("Fabric Engine Example");
    w.set_size(480, 320, WEBVIEW_HINT_NONE);
    w.set_html("Thanks for using webview!");
    w.run();
  } catch (const std::exception &e) {
    std::cerr << "Fatal error: " << e.what() << '\n';
    return 1;
  }

  return 0;
}
