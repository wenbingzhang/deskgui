/**
 * deskgui - A powerful and flexible C++ library to create web-based desktop applications.
 *
 * Copyright (c) 2023 deskgui
 * MIT License
 */

#include "webview_platform_darwin.h"

using namespace deskgui;

using Impl = Webview::Impl;

Impl::Impl(const std::string& name, AppHandler* appHandler, void* window,
           [[maybe_unused]] const WebviewOptions& options)
    : platform_(std::make_unique<Impl::Platform>()), appHandler_(appHandler), name_(name) {
  if (window == nullptr) {
    throw std::invalid_argument("Window is a nullptr");
  }

  platform_->controller = [[WKUserContentController alloc] init];

  platform_->configuration = [[WKWebViewConfiguration alloc] init];
  platform_->configuration.userContentController = platform_->controller;

  platform_->preferences = [[WKPreferences alloc] init];
  platform_->configuration.preferences = platform_->preferences;

  platform_->navigationDelegate = [[CustomNavigationDelegate alloc] initWithWebview:this
                                                                          resources:&resources_];
  [platform_->controller addScriptMessageHandler:platform_->navigationDelegate
                                            name:kScriptMessageCallback];
  [platform_->configuration setURLSchemeHandler:platform_->navigationDelegate
                                   forURLScheme:kSchemeUri];

  auto pool = platform_->configuration.processPool;
  SEL selector = NSSelectorFromString(@"_registerURLSchemeAsSecure:");
  if ([pool respondsToSelector:selector]) {
    NSMethodSignature* signature = [pool methodSignatureForSelector:selector];
    if (signature) {
      NSInvocation* invocation = [NSInvocation invocationWithMethodSignature:signature];
      [invocation setTarget:pool];
      [invocation setSelector:selector];
      id schemeUri = kSchemeUri;
      [invocation setArgument:&schemeUri atIndex:2];
      [invocation invoke];
    }
  }

  const auto nativeDragAndDrop
      = options.getOption<bool>(WebviewOptions::kActivateNativeDragAndDrop);
  platform_->webview = [[CustomWebview alloc] initWithFrame:CGRectZero
                                              configuration:platform_->configuration
                                          enableDragAndDrop:nativeDragAndDrop];
  [platform_->webview setNavigationDelegate:platform_->navigationDelegate];

  // Set up UI delegate
  platform_->uiDelegate = [[CustomUIDelegate alloc] initWithWebview:this];
  [platform_->webview setUIDelegate:platform_->uiDelegate];

  [platform_->webview setFrame:[(__bridge id)window frame]];
  [platform_->webview setValue:@NO forKey:@"drawsBackground"];

  platform_->webview.translatesAutoresizingMaskIntoConstraints = YES;
  [(__bridge id)window addSubview:platform_->webview];

  injectScript(R"(
              window.webview = {
                  async postMessage(message)
                  {
                    webkit.messageHandlers.deskgui_callback.postMessage(message);
                  }
              };
              )");
  show(true);
}

Impl::~Impl() {
  [platform_->webview removeFromSuperview];
  [platform_->controller removeScriptMessageHandlerForName:kScriptMessageCallback];
}

void Impl::enableDevTools(bool state) {
  [platform_->preferences setValue:@(state) forKey:@"developerExtrasEnabled"];
}

void Impl::enableContextMenu(bool state) {
  CustomNavigationDelegate* navigationDelegate
      = static_cast<CustomNavigationDelegate*>(platform_->webview.navigationDelegate);
  navigationDelegate.contextMenuEnabled = state;
  [platform_->webview reload];
}

void Impl::enableZoom(bool state) {
  [platform_->webview setAllowsMagnification:[@(state ? YES : NO) boolValue]];
}

void Impl::enableAcceleratorKeys([[maybe_unused]] bool state) {
  // TODO
}

void Impl::resize(const ViewSize& size) {
  [platform_->webview setFrameSize:NSMakeSize(size.first, size.second)];
}

void Impl::setPosition(const ViewRect& rect) {
  [platform_->webview setFrame:NSMakeRect(rect.L, rect.T, rect.R - rect.L, rect.B - rect.T)];
}

void Impl::show(bool state) { [platform_->webview setHidden:!state]; }

void Impl::navigate(const std::string& url) {
  NSURL* nsURL = [NSURL URLWithString:[NSString stringWithUTF8String:url.c_str()]];
  NSURLRequest* request = [NSURLRequest requestWithURL:nsURL];
  [platform_->webview loadRequest:request];
}

void Impl::loadFile(const std::string& path) {
  NSURL* nsurl = [NSURL fileURLWithPath:[NSString stringWithUTF8String:path.c_str()]
                            isDirectory:FALSE];
  [platform_->webview loadFileURL:nsurl
          allowingReadAccessToURL:[nsurl URLByDeletingLastPathComponent]];
}

std::string Impl::getUrl() {
  NSURL* currentURL = platform_->webview.URL;
  if (currentURL) {
    return std::string(currentURL.absoluteString.UTF8String);
  }
  return "";
}

void Impl::loadResources(Resources&& resources) { resources_ = std::move(resources); }

void Impl::serveResource(const std::string& resourceUrl) { navigate(Impl::kOrigin + resourceUrl); }

void Impl::clearResources() { resources_.clear(); }

void Impl::loadHTMLString(const std::string& html) {
  [platform_->webview loadHTMLString:[NSString stringWithUTF8String:html.c_str()] baseURL:nil];
}

void Impl::injectScript(const std::string& script) {
  WKUserScript* script1 =
      [[WKUserScript alloc] initWithSource:[NSString stringWithUTF8String:script.c_str()]
                             injectionTime:WKUserScriptInjectionTimeAtDocumentStart
                          forMainFrameOnly:YES];
  [platform_->controller addUserScript:script1];
}

void Impl::executeScript(const std::string& script) {
  [platform_->webview evaluateJavaScript:[NSString stringWithUTF8String:script.c_str()]
                       completionHandler:nil];
}
