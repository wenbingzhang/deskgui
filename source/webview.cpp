/**
 * deskgui - A powerful and flexible C++ library to create web-based desktop applications.
 *
 * Copyright (c) 2023 deskgui
 * MIT License
 */

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include "interfaces/webview_impl.h"
#include "utils/dispatch.h"

using namespace deskgui;

Webview::Webview(const std::string& name, AppHandler* appHandler, void* window,
                 const WebviewOptions& options)
    : impl_(std::make_shared<Impl>(name, appHandler, window, options)), events_(&impl_->events()) {}

Webview::~Webview() = default;

std::string Webview::getName() const { return utils::dispatch<&Impl::getName>(impl_); }

void Webview::Impl::addCallback(const std::string& key, MessageCallback callback) {
  callbacks_.try_emplace(key, callback);
}

void Webview::addCallback(const std::string& key, MessageCallback callback) {
  auto script = "window['" + key + "'] = function(payload) { const key = '" + key + "';" +
                R"(
                    window.webview.postMessage({
                              key: key,
                              payload: payload,
                            });
                    }
                )";
  utils::dispatch<&Impl::addCallback>(impl_, key, callback);
  injectScript(script);
  executeScript(script);
}

void Webview::Impl::removeCallback(const std::string& key) { callbacks_.erase(key); }

void Webview::removeCallback(const std::string& key) {
  auto script = "delete window['" + key + "']";
  utils::dispatch<&Impl::removeCallback>(impl_, key);
  injectScript(script);
  executeScript(script);
}

void Webview::postMessage(const std::string& message) {
  executeScript("window.webview.onMessage('" + message + "');");
}

void Webview::Impl::onMessage(const std::string& message) {
  rapidjson::Document doc;
  doc.Parse(message.c_str());
  if (!doc.HasParseError() && doc.IsObject()) {
    if (doc.HasMember("key") && doc.HasMember("payload")) {
      const auto& key = doc["key"];
      if (key.IsString()) {
        std::string keyStr = key.GetString();
        auto callback = callbacks_.find(keyStr);
        if (callback != callbacks_.end()) {
          const auto& payload = doc["payload"];
          rapidjson::StringBuffer buffer;
          rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
          payload.Accept(writer);
          callback->second(buffer.GetString());
        }
      }
    }
  }
  events().emit(deskgui::event::WebviewOnMessage{message});
}

// Settings methods
void Webview::enableDevTools(bool state) { utils::dispatch<&Impl::enableDevTools>(impl_, state); }

void Webview::enableContextMenu(bool state) {
  utils::dispatch<&Impl::enableContextMenu>(impl_, state);
}

void Webview::enableZoom(bool state) { utils::dispatch<&Impl::enableZoom>(impl_, state); }

void Webview::enableAcceleratorKeys(bool state) {
  utils::dispatch<&Impl::enableAcceleratorKeys>(impl_, state);
}

// View methods
void Webview::setPosition(const ViewRect& rect) {
  utils::dispatch<&Impl::setPosition>(impl_, rect);
}

void Webview::show(bool state) { utils::dispatch<&Impl::show>(impl_, state); }

void Webview::resize(const ViewSize& size) { utils::dispatch<&Impl::resize>(impl_, size); }

// Content methods
void Webview::navigate(const std::string& url) { utils::dispatch<&Impl::navigate>(impl_, url); }

void Webview::loadFile(const std::string& path) { utils::dispatch<&Impl::loadFile>(impl_, path); }

void Webview::loadHTMLString(const std::string& html) {
  utils::dispatch<&Impl::loadHTMLString>(impl_, html);
}

void Webview::loadResources(Resources&& resources) {
  utils::dispatch<&Impl::loadResources>(impl_, std::move(resources));
}

void Webview::serveResource(const std::string& resourceUrl) {
  utils::dispatch<&Impl::serveResource>(impl_, resourceUrl);
}

void Webview::clearResources() { utils::dispatch<&Impl::clearResources>(impl_); }

std::string Webview::getUrl() { return utils::dispatch<&Impl::getUrl>(impl_); }

// Functionality methods
void Webview::injectScript(const std::string& script) {
  utils::dispatch<&Impl::injectScript>(impl_, script);
}

void Webview::executeScript(const std::string& script) {
  utils::dispatch<&Impl::executeScript>(impl_, script);
}