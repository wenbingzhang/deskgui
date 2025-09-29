/**
 * deskgui - A powerful and flexible C++ library to create web-based desktop applications.
 *
 * Copyright (c) 2023 deskgui
 * MIT License
 */

#pragma once

#include <AppKit/AppKit.h>

#include "interfaces/window_impl.h"

/**
 * WindowDelegate handles all NSWindow delegate events.
 * It manages:
 * - Window show events
 * - Window close events
 * - Window resize events
 * - Window zoom events
 */
@interface WindowDelegate : NSObject <NSWindowDelegate>
@property(nonatomic, assign) deskgui::Window::Impl* window;
- (instancetype)initWithWindow:(deskgui::Window::Impl*)window;
@end

/**
 * WindowObserver handles all NSWindow notifications.
 * It manages:
 * - Window show notifications
 * - Window close notifications
 * - Window resize notifications
 */
@interface WindowObserver : NSObject
@property(nonatomic, assign) deskgui::Window::Impl* window;
@property(nonatomic, assign) NSWindow* nativeWindow;

- (instancetype)initWithWindow:(deskgui::Window::Impl*)window nativeWindow:(NSWindow*)nativeWindow;
- (void)stopObserving;
@end

namespace deskgui {

  /**
   * Platform details for the Window class.
   * Contains the native NSWindow components used by the Window.
   */
  struct Window::Impl::Platform {
    NSWindow* window = nullptr;  ///< The NSWindow instance
    NSView* view = nullptr;      ///< The content view of the window
    id observer = nullptr;       ///< Window observer for notifications
  };

}  // namespace deskgui
