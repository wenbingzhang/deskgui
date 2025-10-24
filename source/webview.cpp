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

void Webview::Impl::bind(const std::string& key, std::function<std::string(const std::string&)> func) {
  bind_functions_.try_emplace(key, func);
}

void Webview::Impl::unbind(const std::string& key) {
  bind_functions_.erase(key);
}

void Webview::Impl::processPendingResponses() {
  if (!pending_responses_.empty()) {
    for (const auto& response : pending_responses_) {
      executeScript(response);
    }
    pending_responses_.clear();
  }
}

void Webview::bind(const std::string& key, std::function<std::string(const std::string&)> func) {
  // Create JavaScript function that returns a Promise with secure ID generation
  auto script = "window['" + key + "'] = function(payload) { return new Promise((resolve, reject) => {" +
                R"(
                    function generateId() {
                      const crypto = window.crypto || window.msCrypto;
                      const bytes = new Uint8Array(16);
                      crypto.getRandomValues(bytes);
                      return Array.from(bytes)
                        .map(n => n.toString(16).padStart(2, '0'))
                        .join('');
                    }
                    const requestId = generateId();

                    window.webview.postMessage({
                              type: 'bind',
                              key: ')" + key + R"(',
                              payload: payload,
                              requestId: requestId
                            });

                    // Store the promise resolve/reject functions
                    window._bindPromises = window._bindPromises || {};
                    window._bindPromises[requestId] = { resolve, reject };
                });
                })";

  utils::dispatch<&Impl::bind>(impl_, key, func);
  injectScript(script);
  executeScript(script);
}

void Webview::unbind(const std::string& key) {
  // Create script to delete the JavaScript function and clean up any pending promises
  auto script = "delete window['" + key + "'];" +
                "if (window._bindPromises) { " +
                "  for (let requestId in window._bindPromises) { " +
                "    window._bindPromises[requestId].reject('Function unbound'); " +
                "    delete window._bindPromises[requestId]; " +
                "  } " +
                "}";

  utils::dispatch<&Impl::unbind>(impl_, key);
  injectScript(script);
  executeScript(script);
}

void Webview::processPendingResponses() {
  auto& responses = impl_->getPendingResponses();
  if (!responses.empty()) {
    for (const auto& response : responses) {
      executeScript(response);
    }
    responses.clear();
  }
}

void Webview::postMessage(const std::string& message) {
  executeScript("window.webview.onMessage('" + message + "');");
}

void Webview::Impl::onMessage(const std::string& message) {
  rapidjson::Document doc;
  doc.Parse(message.c_str());
  if (!doc.HasParseError() && doc.IsObject()) {
    // Handle bind type messages (for functions that return values)
    if (doc.HasMember("type") && doc["type"].IsString() && std::string(doc["type"].GetString()) == "bind") {
      if (doc.HasMember("key") && doc.HasMember("requestId")) {
        const auto& key = doc["key"];
        const auto& requestId = doc["requestId"];
        if (key.IsString() && requestId.IsString()) {
          std::string keyStr = key.GetString();
          std::string requestIdStr = requestId.GetString();

          auto bind_func = bind_functions_.find(keyStr);
          if (bind_func != bind_functions_.end()) {
            std::string payload;
            if (doc.HasMember("payload")) {
              const auto& payloadVal = doc["payload"];
              rapidjson::StringBuffer buffer;
              rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
              payloadVal.Accept(writer);
              payload = buffer.GetString();
            }

            try {
              std::string result = bind_func->second(payload);
              // Send the result back to JavaScript
              std::string responseScript = "if (window._bindPromises && window._bindPromises['" + requestIdStr + "']) { window._bindPromises['" + requestIdStr + "'].resolve(" + result + "); delete window._bindPromises['" + requestIdStr + "']; }";
              pending_responses_.push_back(responseScript);

              // Process responses immediately to ensure they reach JavaScript
              processPendingResponses();
            } catch (const std::exception& e) {
              std::string errorScript = "if (window._bindPromises && window._bindPromises['" + requestIdStr + "']) { window._bindPromises['" + requestIdStr + "'].reject('" + std::string(e.what()) + "'); delete window._bindPromises['" + requestIdStr + "']; }";
              pending_responses_.push_back(errorScript);

              // Process error responses immediately
              processPendingResponses();
            }
          }
        }
      }
    }
    // Handle regular callback messages
    else if (doc.HasMember("key") && doc.HasMember("payload")) {
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