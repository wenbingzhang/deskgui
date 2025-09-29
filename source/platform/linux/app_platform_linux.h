/**
 * deskgui - A powerful and flexible C++ library to create web-based desktop applications.
 *
 * Copyright (c) 2023 deskgui
 * MIT License
 */

#pragma once

#include <gtk/gtk.h>

#include "interfaces/app_impl.h"

namespace deskgui {
  class App::Impl::Platform {
  public:
    Platform() = default;
    ~Platform() = default;
  };
}  // namespace deskgui
