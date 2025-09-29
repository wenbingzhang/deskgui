/**
 * deskgui - A powerful and flexible C++ library to create web-based desktop applications.
 *
 * Copyright (c) 2023 deskgui
 * MIT License
 */

#include "app_platform_linux.h"
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
  gtk_main();
}

void Impl::terminate() {
  if (isRunning_.load()) {
    isRunning_.store(false);
    gtk_main_quit();
  }
}

void Impl::dispatch(DispatchTask&& task) {
  g_idle_add(
      [](gpointer user_data) -> gboolean {
        DispatchTask* task = static_cast<DispatchTask*>(user_data);
        (*task)();
        delete task;
        return G_SOURCE_REMOVE;
      },
      new DispatchTask(std::move(task)));
}