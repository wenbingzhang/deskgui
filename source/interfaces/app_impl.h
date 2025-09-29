/**
 * deskgui - A powerful and flexible C++ library to create web-based desktop applications.
 *
 * Copyright (c) 2023 deskgui
 * MIT License
 */

#pragma once

#include <deskgui/app.h>

#include <atomic>
#include <future>
#include <set>
#include <thread>

namespace deskgui {
  class App::Impl {
  public:
    class Platform;

    explicit Impl(const std::string& name);
    ~Impl();

    [[nodiscard]] Window* createWindow(const std::string& name, AppHandler* appHandler,
                                       void* nativeWindow = nullptr);
    void destroyWindow(const std::string& name);
    [[nodiscard]] Window* getWindow(const std::string& name) const;

    [[nodiscard]] inline std::string_view getName() const { return name_; }

    void run();
    void terminate();
    [[nodiscard]] inline bool isRunning() const { return isRunning_.load(); }

    [[nodiscard]] inline bool isMainThread() const {
      return std::this_thread::get_id() == mainThreadId_;
    }
    void dispatch(DispatchTask&& task);

  private:
    std::unique_ptr<Platform> platform_{nullptr};

    std::string name_;
    std::atomic<bool> isRunning_{false};
    std::thread::id mainThreadId_;

    std::unordered_map<std::string, std::unique_ptr<Window>> windows_;
  };
}  // namespace deskgui
