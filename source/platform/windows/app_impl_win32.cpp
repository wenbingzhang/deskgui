/**
 * deskgui - A powerful and flexible C++ library to create web-based desktop applications.
 *
 * Copyright (c) 2023 deskgui
 * MIT License
 */

#include <windows.h>

#include "app_platform_win32.h"
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

  MSG msg = {};
  while (isRunning_.load()) {
    WaitMessage();

    // Process messages
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
      if (msg.message == WM_QUIT) {
        Impl::terminate();
        break;
      };
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
  }
}

void Impl::terminate() { isRunning_.store(false); }

void Impl::dispatch(DispatchTask&& task) {
  auto* heapTask = new DispatchTask(std::move(task));
  PostMessage(platform_->messageWindow, Platform::windowMessage, 0,
              reinterpret_cast<LPARAM>(heapTask));
}