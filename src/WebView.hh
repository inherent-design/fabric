#pragma once

#include "webview/webview.h"
#include <string>

namespace Fabric {

/**
 * @brief WebView wrapper class for the Fabric Engine
 *
 * This class encapsulates the webview functionality and provides
 * a clean interface for working with the embedded web browser.
 */
class WebView {
public:
  /**
   * @brief Construct a new WebView object
   *
   * @param debug Enable debug mode (shows developer tools)
   * @param window Parent window handle (nullptr for default)
   */
  WebView(bool debug = false, void *window = nullptr);

  /**
   * @brief Set the window title
   *
   * @param title Window title
   */
  void setTitle(const std::string &title);

  /**
   * @brief Set the window size
   *
   * @param width Window width
   * @param height Window height
   * @param hint Size hint (WEBVIEW_HINT_NONE, WEBVIEW_HINT_MIN,
   * WEBVIEW_HINT_MAX, WEBVIEW_HINT_FIXED)
   */
  void setSize(int width, int height, webview_hint_t hint = WEBVIEW_HINT_NONE);

  /**
   * @brief Navigate to a URL
   *
   * @param url URL to navigate to
   */
  void navigate(const std::string &url);

  /**
   * @brief Set HTML content directly
   *
   * @param html HTML content
   */
  void setHtml(const std::string &html);

  /**
   * @brief Run the main event loop
   */
  void run();

  /**
   * @brief Terminate the main event loop
   */
  void terminate();

  /**
   * @brief Evaluate JavaScript in the webview
   *
   * @param js JavaScript code to evaluate
   */
  void eval(const std::string &js);

  /**
   * @brief Bind a native C++ callback to be callable from JavaScript
   *
   * @param name Name of the function in JavaScript
   * @param fn Callback function
   */
  void bind(const std::string &name,
            const std::function<std::string(const std::string &)> &fn);

private:
  webview::webview webview_;
};

} // namespace Fabric
