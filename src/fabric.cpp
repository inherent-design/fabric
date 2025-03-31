#include "ArgumentParser.h"
#include "Constants.g.h"
#include "webview/webview.h"
#include <iostream>

#if defined(_WIN32)
#include <windows.h>
int WINAPI WinMain(HINSTANCE /*hInst*/, HINSTANCE /*hPrevInst*/,
                   LPSTR /*lpCmdLine*/, int /*nCmdShow*/) {
#else
int main(int argc, char *argv[]) {
#endif
  try {
    // Define common CLI arguments using ArgumentParserBuilder
    ArgumentParserBuilder builder;
    builder.addOption("--version", TokenType::LiteralString, true)
        .addOption("--help", TokenType::LiteralString, true)
        .addOption("--testValue", TokenType::LiteralFloat, false);

    ArgumentParser parser = builder.build();

#if defined(_WIN32)
    bool debug = false;
#else
    bool debug = true;
#endif

    if (argc > 1) {
      parser.parse(argc, argv);

      // Check for --help argument
      if (parser.getArgument("--help")) {
        ArgumentParser::printHelp();
        return 0;
      }

      // Check for --version argument
      if (parser.getArgument("--version")) {
        ArgumentParser::printVersion();
        return 0;
      }

      // Repeat similar checks for other arguments
      // ...
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
