/**
 * deskgui - A powerful and flexible C++ library to create web-based desktop applications.
 *
 * Copyright (c) 2023 deskgui
 * MIT License
 */

#pragma once

#include <gtk/gtk.h>
#include <webkit2/webkit2.h>

#include <algorithm>

#include "interfaces/webview_impl.h"

namespace deskgui {
  struct Webview::Impl::Platform {
    WebKitWebView* webview;
    GtkFixed* container;

    static gboolean onNavigationRequest(WebKitWebView* webview, WebKitPolicyDecision* decision,
                                        WebKitPolicyDecisionType decisionType, Webview* self);
    static void onLoadChanged(WebKitWebView* webview, WebKitLoadEvent loadEvent, Webview* self);
    static void onScriptMessageReceived(WebKitUserContentManager* manager,
                                        WebKitJavascriptResult* message, Webview* self);
    static void onCustomSchemeRequest(WebKitURISchemeRequest* request, gpointer userData);
  };
}  // namespace deskgui
