/**
 * deskgui - A powerful and flexible C++ library to create web-based desktop applications.
 *
 * Copyright (c) 2023 deskgui
 * MIT License
 */

#include <windows.h>

#include "interfaces/app_impl.h"

namespace deskgui {
  class App::Impl::Platform {
  public:
    Platform();
    ~Platform() = default;

    static LRESULT CALLBACK windowMessageProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static const inline UINT windowMessage = RegisterWindowMessageW(L"window_message");

    HWND messageWindow;
  };
}  // namespace deskgui