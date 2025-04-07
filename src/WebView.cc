#include "WebView.hh"

namespace Fabric {

WebView::WebView(bool debug, void *window) : webview_(debug, window) {}

void WebView::setTitle(const std::string &title) {
  webview_.set_title(title.c_str());
}

void WebView::setSize(int width, int height, webview_hint_t hint) {
  webview_.set_size(width, height, hint);
}

void WebView::navigate(const std::string &url) {
  webview_.navigate(url.c_str());
}

void WebView::setHtml(const std::string &html) {
  webview_.set_html(html.c_str());
}

void WebView::run() { webview_.run(); }

void WebView::terminate() { webview_.terminate(); }

void WebView::eval(const std::string &js) { webview_.eval(js.c_str()); }

void WebView::bind(const std::string &name,
                   const std::function<std::string(const std::string &)> &fn) {
  webview_.bind(name.c_str(), [fn](const std::string &req) -> std::string {
    return fn(req);
  });
}

} // namespace Fabric
