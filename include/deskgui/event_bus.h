/**
 * deskgui - A powerful and flexible C++ library to create web-based desktop applications.
 *
 * Copyright (c) 2023 deskgui
 * MIT License
 */

#pragma once

#include <deskgui/events.h>

#include <functional>
#include <mutex>
#include <shared_mutex>
#include <type_traits>
#include <typeindex>
#include <unordered_map>
#include <utility>

namespace deskgui {

  class EventBus {
  public:
    // Generic connect for any callable (lambda, functor, std::function)
    template <class EventType, typename Callable>
    [[maybe_unused]] UniqueId connect(Callable&& listener) {
      std::unique_lock lock(mutex_);

      return connectHelper<EventType>([cb = std::forward<Callable>(listener)](void* event) mutable {
        callHelper(cb, static_cast<EventType*>(event));
      });
    }

    template <typename EventType> void disconnect(UniqueId id) {
      std::unique_lock lock(mutex_);

      const auto typeId = std::type_index(typeid(EventType));
      const auto it = connections_.find(typeId);
      if (it != connections_.end()) {
        it->second.erase(id);
      }
    }

    template <class EventType, typename = std::enable_if_t<!std::is_pointer_v<EventType>>>
    void emit(EventType&& event) {
      std::shared_lock lock(mutex_);

      const auto typeId = std::type_index(typeid(EventType));
      const auto it = connections_.find(typeId);
      if (it != connections_.end()) {
        for (const auto& [key, callback] : it->second) {
          callback(&event);
        }
      }
    }

    template <class EventType> [[nodiscard]] std::size_t count() const {
      std::shared_lock lock(mutex_);
      const auto typeId = std::type_index(typeid(EventType));
      const auto it = connections_.find(typeId);
      return (it != connections_.end()) ? it->second.size() : 0;
    }

    void clear() {
      std::unique_lock lock(mutex_);
      connections_.clear();
    }

  private:
    template <class EventType, class ListenerCallback>
    UniqueId connectHelper(ListenerCallback&& listener) {
      const auto id = EventListenerId::newId();
      const auto typeId = std::type_index(typeid(EventType));

      auto& callbacks_ = connections_[typeId];
      callbacks_.try_emplace(id, std::forward<ListenerCallback>(listener));

      return id;
    }

    // Detects whether Callable wants EventType& or nothing
    template <typename Callable, typename EventPtr>
    static auto callHelper(Callable& cb, EventPtr event) -> decltype(cb(*event), void()) {
      cb(*event);  // callable expects EventType&
    }

    template <typename Callable, typename EventPtr>
    static auto callHelper(Callable& cb, EventPtr) -> decltype(cb(), void()) {
      cb();  // callable expects no arguments
    }

    mutable std::shared_mutex mutex_;

    using EventCallback = std::function<void(void*)>;
    using Connections = std::unordered_map<UniqueId, EventCallback>;
    using EventConnections = std::unordered_map<std::type_index, Connections>;

    EventConnections connections_;
  };

}  // namespace deskgui
