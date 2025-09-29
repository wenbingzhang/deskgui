/**
 * deskgui - A powerful and flexible C++ library to create web-based desktop applications.
 *
 * Copyright (c) 2023 deskgui
 * MIT License
 */

#include "app_platform_win32.h"

using namespace deskgui;

using Platform = App::Impl::Platform;

LRESULT CALLBACK Platform::windowMessageProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  if (uMsg == Platform::windowMessage) {
    auto* task = reinterpret_cast<DispatchTask*>(lParam);
    if (task) {
      (*task)();
      delete task;  // clean up after running
    }
    return 0;
  }
  return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

Platform::Platform() {
  // Register class for the message-only window
  WNDCLASSEX wc = {sizeof(WNDCLASSEX),       0,       windowMessageProc, 0,       0,
                   GetModuleHandle(nullptr), nullptr, nullptr,           nullptr, nullptr,
                   L"MessageWindowClass",    nullptr};
  RegisterClassEx(&wc);

  // Create the message-only window
  messageWindow = CreateWindowEx(0, L"MessageWindowClass", L"MessageWindow", 0, 0, 0, 0, 0,
                                 HWND_MESSAGE, nullptr, nullptr, nullptr);
}
