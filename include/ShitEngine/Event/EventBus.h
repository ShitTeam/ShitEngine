#pragma once
#include "../Core/Core.h"
#include <functional>
#include <typeindex>
#include <unordered_map>
#include <vector>
#include <memory>

namespace Shit {

/**
 * @brief 事件总线，提供类型安全的事件发布/订阅
 *
 * 用法：
 *   Shit::EventBus::Listen<CollisionEvent>([](const CollisionEvent& e) { ... });
 *   Shit::EventBus::Emit<CollisionEvent>({a, b});
 */
class SHIT_API EventBus {
public:
    using HandlerID = size_t;

    // --- 静态API ---
    template<typename Event>
    static HandlerID Listen(std::function<void(const Event&)> callback) {
        return GetInstance().listen<Event>(std::move(callback));
    }

    template<typename Event>
    static HandlerID ListenOnce(std::function<void(const Event&)> callback) {
        auto& instance = GetInstance();
        auto weak = std::make_shared<std::function<void(const Event&)>>(std::move(callback));
        return instance.listen<Event>([weak, &instance](const Event& e) {
            if (auto f = *weak) { f(e); }
        });
    }

    template<typename Event>
    static void Emit(const Event& event) {
        GetInstance().emit<Event>(event);
    }

    template<typename Event>
    static void Unbind(HandlerID id) {
        GetInstance().unbind<Event>(id);
    }

    template<typename Event>
    static void Clear() {
        GetInstance().clear<Event>();
    }

    static void ClearAll() {
        GetInstance().clearAll();
    }

    // --- 单例 ---
    static EventBus& GetInstance();

    EventBus(const EventBus&) = delete;
    EventBus& operator=(const EventBus&) = delete;
    EventBus(EventBus&&) = delete;
    EventBus& operator=(EventBus&&) = delete;

private:
    EventBus() = default;
    ~EventBus() = default;

    struct IHandlerList {
        virtual ~IHandlerList() = default;
    };

    template<typename Event>
    struct HandlerList : IHandlerList {
        struct Entry {
            HandlerID id;
            std::function<void(const Event&)> callback;
        };
        std::vector<Entry> handlers;
        HandlerID nextID = 0;
    };

    template<typename Event>
    HandlerList<Event>& getOrCreateList() {
        auto key = std::type_index(typeid(Event));
        auto it = m_handlers.find(key);
        if (it == m_handlers.end()) {
            auto list = std::make_unique<HandlerList<Event>>();
            auto* ptr = list.get();
            m_handlers[key] = std::move(list);
            return *ptr;
        }
        return static_cast<HandlerList<Event>&>(*it->second);
    }

    template<typename Event>
    HandlerID listen(std::function<void(const Event&)> callback) {
        auto& list = getOrCreateList<Event>();
        HandlerID id = list.nextID++;
        list.handlers.push_back({id, std::move(callback)});
        return id;
    }

    template<typename Event>
    void emit(const Event& event) {
        auto it = m_handlers.find(std::type_index(typeid(Event)));
        if (it == m_handlers.end()) return;
        auto& list = static_cast<HandlerList<Event>&>(*it->second);
        for (auto& entry : list.handlers) {
            entry.callback(event);
        }
    }

    template<typename Event>
    void unbind(HandlerID id) {
        auto it = m_handlers.find(std::type_index(typeid(Event)));
        if (it == m_handlers.end()) return;
        auto& list = static_cast<HandlerList<Event>&>(*it->second);
        list.handlers.erase(
            std::remove_if(list.handlers.begin(), list.handlers.end(),
                [id](const auto& entry) { return entry.id == id; }),
            list.handlers.end()
        );
    }

    template<typename Event>
    void clear() {
        m_handlers.erase(std::type_index(typeid(Event)));
    }

    void clearAll() {
        m_handlers.clear();
    }

    std::unordered_map<std::type_index, std::unique_ptr<IHandlerList>> m_handlers;
};

} // namespace Shit
