/**
 * deskgui - A powerful and flexible C++ library to create web-based desktop applications.
 *
 * Copyright (c) 2023 deskgui
 * MIT License
 */

#include "interfaces/app_impl.h"

using namespace deskgui;

App::App(const std::string& name) : impl_(std::make_unique<Impl>(name)) {}

App::~App() = default;

Window* App::Impl::createWindow(const std::string& name, AppHandler* appHandler,
                                void* nativeWindow) {
  try {
    auto result = windows_.try_emplace(
        name, std::unique_ptr<Window>(new Window(name, appHandler, nativeWindow)));
    if (result.second) {
      return result.first->second.get();
    }
    return nullptr;
  } catch ([[maybe_unused]] const std::exception& e) {
    return nullptr;
  }
}

Window* App::createWindow(const std::string& name, void* nativeWindow) {
  if (!isMainThread()) {
    return dispatchOnMainThread(
        [this, name, nativeWindow] { return createWindow(name, nativeWindow); });
  }
  return impl_->createWindow(name, getHandler(), nativeWindow);
}

void App::Impl::destroyWindow(const std::string& name) {
  if (auto it = windows_.find(name); it != windows_.end()) {
    windows_.erase(it);

    if (windows_.empty()) {
      terminate();
    }
  }
}

void App::destroyWindow(const std::string& name) {
  if (!isMainThread()) {
    return dispatchOnMainThread([this, name] { destroyWindow(name); });
  }
  impl_->destroyWindow(name);
}

Window* App::Impl::getWindow(const std::string& name) const {
  if (auto it = windows_.find(name); it != windows_.end()) {
    return it->second.get();
  } else {
    return nullptr;
  }
}

Window* App::getWindow(const std::string& name) const {
  if (!isMainThread()) {
    return dispatchOnMainThread([this, name] { return getWindow(name); });
  }
  return impl_->getWindow(name);
}

std::string_view App::getName() const { return impl_->getName(); }

bool App::isRunning() const { return impl_->isRunning(); }

void App::run() { impl_->run(); }

void App::terminate() {
  if (!isMainThread()) {
    return dispatchOnMainThread([this]() { terminate(); });
  }
  impl_->terminate();
}

bool App::isMainThread() const { return impl_->isMainThread(); }

void App::dispatch(DispatchTask&& task) const { impl_->dispatch(std::move(task)); }

void App::notifyWindowClosedFromUI(const std::string& name) {
  if (!isMainThread()) {
    return dispatchOnMainThread([this, name] { notifyWindowClosedFromUI(name); });
  }
  destroyWindow(name);
}