#include "webview/webview.h"

#include <iostream>

#if defined(_WIN32)
#include <windows.h>
int WINAPI WinMain(HINSTANCE /*hInst*/, HINSTANCE /*hPrevInst*/,
                   LPSTR /*lpCmdLine*/, int /*nCmdShow*/) {
#else
int main() {
#endif
  try {
    // Set debug=true on Linux/macOS for console logs
#if defined(_WIN32)
    bool debug = false;
#else
    bool debug = true;
#endif

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
