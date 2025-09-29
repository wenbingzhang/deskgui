/**
 * deskgui - A powerful and flexible C++ library to create web-based desktop applications.
 *
 * Copyright (c) 2023 deskgui
 * MIT License
 */

#pragma once

#include <deskgui/event_bus.h>
#include <deskgui/webview.h>

#include <unordered_map>


namespace deskgui {

  class Webview::Impl {
    class Platform;

  public:
    explicit Impl(const std::string& name, AppHandler* appHandler, void* window,
                  const WebviewOptions& options);
    ~Impl();

    /**
     * Constants defining the protocol, host, and origin URL of the URL scheme
     * used in the webview to serve custom resources.
     */
    static constexpr auto kProtocol = "webview";
    inline static const std::string kOrigin = "webview://localhost/";
    inline static const std::wstring kWOrigin = L"webview://localhost/";

    [[nodiscard]] inline std::string getName() const { return name_; }
    
    // Settings
    void enableDevTools(bool state);
    void enableContextMenu(bool state);
    void enableZoom(bool state);
    void enableAcceleratorKeys(bool state);

    // View
    void setPosition(const ViewRect& rect);
    void show(bool state);
    void resize(const ViewSize& size);

    // Content
    void navigate(const std::string& url);
    void loadFile(const std::string& path);
    void loadHTMLString(const std::string& html);
    void loadResources(Resources&& resources);
    void serveResource(const std::string& resourceUrl);
    void clearResources();
    [[nodiscard]] std::string getUrl();

    // Functionality
    void addCallback(const std::string& key, MessageCallback callback);
    void removeCallback(const std::string& key);
    void postMessage(const std::string& message);
    void injectScript(const std::string& script);
    void executeScript(const std::string& script);
    void onMessage(const std::string& message);

    [[nodiscard]] inline AppHandler* application() const { return appHandler_; }
    [[nodiscard]] inline EventBus& events() { return events_; }

  private:
    std::unique_ptr<Platform> platform_{nullptr};
    std::string name_;
    std::unordered_map<std::string, MessageCallback> callbacks_;
    AppHandler* appHandler_{nullptr};
    Resources resources_;
    EventBus events_;
  };

}  // namespace deskgui
