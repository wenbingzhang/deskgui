/**
 * deskgui - A powerful and flexible C++ library to create web-based desktop applications.
 *
 * Copyright (c) 2023 deskgui
 * MIT License
 */

#include <shlwapi.h>

#include <iostream>

#include "js/drop.h"
#include "utils/strings.h"
#include "webview_platform_win32.h"

using namespace deskgui;
using namespace deskgui::utils;
using namespace Microsoft::WRL;

using Impl = Webview::Impl;

Impl::Impl(const std::string& name, AppHandler* appHandler, void* window,
           const WebviewOptions& options)
    : platform_(std::make_unique<Impl::Platform>()), name_(name), appHandler_(appHandler) {
  auto hwnd = static_cast<HWND>(window);
  if (hwnd == nullptr) {
    throw std::invalid_argument("Window is a nullptr");
  }

  if (!platform_->createWebviewInstance(name_, hwnd, options)) {
    throw std::exception("Cannot initialize webview");
  }

  // Message received event
  platform_->webview->add_WebMessageReceived(
      Callback<ICoreWebView2WebMessageReceivedEventHandler>(
          [this]([[maybe_unused]] ICoreWebView2* sender,
                 ICoreWebView2WebMessageReceivedEventArgs* args) {
            if (platform_->handleDragAndDrop(args)) {
              return S_OK;
            }

            wil::unique_cotaskmem_string message;
            args->get_WebMessageAsJson(&message);
            onMessage(ws2s(message.get()));
            return S_OK;
          })
          .Get(),
      nullptr);

  // Handle the navigation starting event
  platform_->webview->add_NavigationStarting(
      Callback<ICoreWebView2NavigationStartingEventHandler>(
          [=](ICoreWebView2* sender, ICoreWebView2NavigationStartingEventArgs* args) -> HRESULT {
            wil::unique_cotaskmem_string uri;
            args->get_Uri(&uri);
            event::WebviewNavigationStarting event(ws2s(uri.get()));
            events_.emit(event);

            if (event.isCancelled()) {
              sender->Stop();
            }
            return S_OK;
          })
          .Get(),
      nullptr);

  // Handle frame navigation starting event (if needed)
  platform_->webview->add_FrameNavigationStarting(
      Callback<ICoreWebView2NavigationStartingEventHandler>(
          [=](ICoreWebView2* sender, ICoreWebView2NavigationStartingEventArgs* args) -> HRESULT {
            wil::unique_cotaskmem_string uri;
            args->get_Uri(&uri);

            event::WebviewFrameNavigationStarting event(ws2s(uri.get()));
            events_.emit(event);

            if (event.isCancelled()) {
              sender->Stop();
            }
            return S_OK;
          })
          .Get(),
      nullptr);

  // Handle navigation completed event
  platform_->webview->add_NavigationCompleted(
      Callback<ICoreWebView2NavigationCompletedEventHandler>(
          [=]([[maybe_unused]] ICoreWebView2* sender,
              ICoreWebView2NavigationCompletedEventArgs* args) -> HRESULT {
            BOOL success;
            args->get_IsSuccess(&success);
            events_.emit(event::WebviewContentLoaded(static_cast<bool>(success)));
            return S_OK;
          })
          .Get(),
      nullptr);

  // Handle source changed event
  platform_->webview->add_SourceChanged(
      Callback<ICoreWebView2SourceChangedEventHandler>(
          [=]([[maybe_unused]] ICoreWebView2* sender,
              [[maybe_unused]] ICoreWebView2SourceChangedEventArgs* args) -> HRESULT {
            events_.emit(event::WebviewSourceChanged(getUrl()));
            return S_OK;
          })
          .Get(),
      nullptr);

  platform_->webview->add_NewWindowRequested(
      Callback<ICoreWebView2NewWindowRequestedEventHandler>(
          [=](ICoreWebView2* sender, ICoreWebView2NewWindowRequestedEventArgs* args) -> HRESULT {
            wil::unique_cotaskmem_string uri;
            args->get_Uri(&uri);

            event::WebviewWindowRequested event(ws2s(uri.get()));
            events_.emit(event);

            if (event.isCancelled()) {
              args->put_Handled(true);
              return S_OK;
            }

            return S_OK;
          })
          .Get(),
      nullptr);

  injectScript(R"(
                window.webview = {
                    async postMessage(message) 
                    {
                        window.chrome.webview.postMessage(message);
                    }
                };
                )");

  if (options.getOption<bool>(WebviewOptions::kActivateNativeDragAndDrop)) {
    injectScript(js::kWindowsDropListener);
  }

  enableAcceleratorKeys(false);
  show(true);
}

Impl::~Impl() = default;

void Impl::enableDevTools(bool state) {
  wil::com_ptr<ICoreWebView2Settings> settings;
  platform_->webview->get_Settings(&settings);
  settings->put_AreDevToolsEnabled(state);
  platform_->webview->Reload();
}

void Impl::enableContextMenu(bool state) {
  wil::com_ptr<ICoreWebView2Settings> settings;
  platform_->webview->get_Settings(&settings);
  settings->put_AreDefaultContextMenusEnabled(state);
  platform_->webview->Reload();
}

void Impl::enableZoom(bool state) {
  wil::com_ptr<ICoreWebView2Settings> settings;
  platform_->webview->get_Settings(&settings);
  settings->put_IsZoomControlEnabled(state);
  platform_->webview->Reload();
}

void Impl::enableAcceleratorKeys(bool state) {
  if (state) {
    if (platform_->acceleratorKeysToken) {
      platform_->webviewController->remove_AcceleratorKeyPressed(
          platform_->acceleratorKeysToken.value());
      platform_->acceleratorKeysToken.reset();
    }
  } else {
    if (!platform_->acceleratorKeysToken) {
      platform_->acceleratorKeysToken = EventRegistrationToken();

      platform_->webviewController->add_AcceleratorKeyPressed(
          Callback<ICoreWebView2AcceleratorKeyPressedEventHandler>(
              [this](ICoreWebView2Controller* sender,
                     ICoreWebView2AcceleratorKeyPressedEventArgs* args) -> HRESULT {
                wil::com_ptr<ICoreWebView2AcceleratorKeyPressedEventArgs2> args2;
                args->QueryInterface(IID_PPV_ARGS(&args2));
                if (args2) {
                  args2->put_IsBrowserAcceleratorKeyEnabled(FALSE);
                }
                return S_OK;
              })
              .Get(),
          &platform_->acceleratorKeysToken.value());
    }
  }
}

void Impl::resize(const ViewSize& size) {
  if (platform_->webviewController) {
    platform_->webviewController->put_Bounds(
        RECT{0, 0, static_cast<LONG>(size.first), static_cast<LONG>(size.second)});
  }
}

void Impl::setPosition(const ViewRect& rect) {
  if (platform_->webviewController) {
    platform_->webviewController->put_Bounds(
        RECT{static_cast<LONG>(rect.L), static_cast<LONG>(rect.T), static_cast<LONG>(rect.R),
             static_cast<LONG>(rect.B)});
  }
}

void Impl::show(bool state) {
  if (platform_->webviewController) {
    platform_->webviewController->put_IsVisible(static_cast<BOOL>(state));
  }
}

void Impl::navigate(const std::string& url) { platform_->webview->Navigate(s2ws(url).c_str()); }

void Impl::loadFile(const std::string& path) {
  std::string filePath = "file://" + path;
  platform_->webview->Navigate(s2ws(filePath).c_str());
}

void Impl::loadHTMLString(const std::string& html) {
  platform_->webview->NavigateToString(s2ws(html).c_str());
}

void Impl::loadResources(Resources&& resources) {
  resources_ = std::move(resources);

  if (!platform_->webResourceRequestedToken) {
    platform_->webResourceRequestedToken = EventRegistrationToken();

    platform_->webview->AddWebResourceRequestedFilter((Impl::kWOrigin + L"*").c_str(),
                                                      COREWEBVIEW2_WEB_RESOURCE_CONTEXT_ALL);

    platform_->webview->add_WebResourceRequested(
        Callback<ICoreWebView2WebResourceRequestedEventHandler>(
            [=](ICoreWebView2* sender,
                ICoreWebView2WebResourceRequestedEventArgs* args) -> HRESULT {
              // Get the request object
              wil::com_ptr<ICoreWebView2WebResourceRequest> request;
              HRESULT hr = args->get_Request(&request);
              if (FAILED(hr)) {
                return hr;
              }

              // Get the URL of the requested resource
              wil::unique_cotaskmem_string url;
              hr = request->get_Uri(&url);
              if (FAILED(hr)) {
                return hr;
              }

              std::string requestedUrl = ws2s(url.get());

              // Check if the requested URL matches the resource you want to load
              auto it = std::find_if(resources_.begin(), resources_.end(),
                                     [&](const Resource& resource) {
                                       return (Impl::kOrigin + resource.scheme) == requestedUrl;
                                     });

              if (it != resources_.end()) {
                auto webview2 = platform_->webview.try_query<ICoreWebView2_2>();
                if (webview2) {
                  wil::com_ptr<ICoreWebView2Environment> env;
                  webview2->get_Environment(&env);

                  // Create an IStream object from the content
                  wil::com_ptr<IStream> contentStream
                      = SHCreateMemStream(reinterpret_cast<const BYTE*>(it->content.data()),
                                          static_cast<UINT>(it->content.size()));

                  wil::com_ptr<ICoreWebView2WebResourceResponse> response;
                  hr = env->CreateWebResourceResponse(contentStream.get(), 200, L"OK",
                                                      s2ws("Content-Type:" + it->mime).c_str(),
                                                      &response);
                  if (FAILED(hr)) {
                    return hr;
                  }

                  args->put_Response(response.get());
                }
              }

              return S_OK;
            })
            .Get(),
        &platform_->webResourceRequestedToken.value());
  }
}

void Impl::serveResource(const std::string& resourceUrl) { navigate(Impl::kOrigin + resourceUrl); }

void Impl::clearResources() {
  resources_.clear();

  if (platform_->webResourceRequestedToken) {
    platform_->webview->remove_WebResourceRequested(platform_->webResourceRequestedToken.value());
    platform_->webview->RemoveWebResourceRequestedFilter((Impl::kWOrigin + L"*").c_str(),
                                                         COREWEBVIEW2_WEB_RESOURCE_CONTEXT_ALL);
    platform_->webResourceRequestedToken.reset();
  }
}

std::string Impl::getUrl() {
  wil::unique_cotaskmem_string url;
  platform_->webview->get_Source(&url);
  return ws2s(url.get());
}

void Impl::injectScript(const std::string& script) {
  platform_->webview->AddScriptToExecuteOnDocumentCreated(s2ws(script).c_str(), nullptr);
}

void Impl::executeScript(const std::string& script) {
  platform_->webview->ExecuteScript(s2ws(script).c_str(), nullptr);
}