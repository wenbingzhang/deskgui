/**
 * deskgui - A powerful and flexible C++ library to create web-based desktop applications.
 *
 * Copyright (c) 2023 deskgui
 * MIT License
 */

#import <Cocoa/Cocoa.h>

#include "app_platform_darwin.h"
#include "interfaces/app_impl.h"

using namespace deskgui;

using Impl = App::Impl;

Impl::Impl(const std::string& name) : platform_(std::make_unique<Impl::Platform>()), name_(name) {
  mainThreadId_ = std::this_thread::get_id();
}

Impl::~Impl() {
  if (isRunning_.load()) {
    terminate();
  }
}

void Impl::run() {
  if (isRunning_.load()) {
    return;
  }

  isRunning_.store(true);
  mainThreadId_ = std::this_thread::get_id();

  @autoreleasepool {
    [NSApplication sharedApplication];
    [NSApp run];
  }
}

void Impl::terminate() {
  if (isRunning_.load()) {
    isRunning_.store(false);
    [NSApp terminate:nil];
  }
}

void Impl::dispatch(DispatchTask&& task) {
  auto t = std::make_shared<DispatchTask>(std::move(task));
  dispatch_async(dispatch_get_main_queue(), ^{
    (*t)();
  });
}