/**
 * deskgui - A powerful and flexible C++ library to create web-based desktop applications.
 *
 * Copyright (c) 2023 deskgui
 * MIT License
 */

#pragma once

#include <deskgui/app_handler.h>

#include <functional>
#include <future>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>

namespace deskgui {
  namespace utils {

    template <auto Func, typename Impl, typename... Args>
    auto dispatch(const std::shared_ptr<Impl>& impl, Args&&... args) {
      using ReturnType = std::invoke_result_t<decltype(Func), Impl*, Args...>;

      auto defaultReturn = []() -> ReturnType {
        if constexpr (!std::is_void_v<ReturnType>) {
          return ReturnType{};
        } else {
          return;
        }
      };

      if (!impl) return defaultReturn();

      auto* app = reinterpret_cast<AppHandler*>(impl->application());
      if (!app) return defaultReturn();

      if (app->isMainThread()) {
        return std::invoke(Func, impl.get(), std::forward<Args>(args)...);
      }

      std::weak_ptr<Impl> weakImpl = impl;

      return app->dispatchOnMainThread([weakImpl,
                                        argsTuple = std::make_tuple(std::forward<Args>(args)...),
                                        defaultReturn]() mutable {
        auto sharedImpl = weakImpl.lock();
        if (!sharedImpl) return defaultReturn();

        return std::apply(
            [&sharedImpl](auto&&... unpackedArgs) {
              return std::invoke(Func, sharedImpl.get(),
                                 std::forward<decltype(unpackedArgs)>(unpackedArgs)...);
            },
            std::move(argsTuple));
      });
    }

  }  // namespace utils
}  // namespace deskgui
