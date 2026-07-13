#pragma once
#include "Event.h"
#include <functional>
#include <typeindex>
#include <unordered_map>
#include <vector>
#include <memory>
#include <queue>
#include <mutex>

namespace Shit {

/**
 * @brief 事件总线，缓冲队列模式
 *
 * 所有事件先入队，在游戏循环的统一时刻（ProcessEvents）派发。
 * 避免递归触发、迭代器失效等问题。
 *
 * 使用示例：
 *   struct CollisionEvent : public Shit::Event { class GameObject* a; class GameObject* b; };
 *
 *   uint64_t id = Shit::EventBus::Subscribe<CollisionEvent>(
 *       [](const CollisionEvent& e) { /* handle *\/ });
 *
 *   Shit::EventBus::Emit(CollisionEvent{nullptr, nullptr});
 *   // 游戏循环结束时：
 *   Shit::EventBus::ProcessEvents();
 *   Shit::EventBus::Unsubscribe<CollisionEvent>(id);
 *   Shit::EventBus::Unsubscribe<CollisionEvent>(id);
 */
class SHIT_API EventBus {
public:
    using HandlerID = uint64_t;

    template<typename EventType>
    static HandlerID Subscribe(std::function<void(const EventType&)> callback) {
        static_assert(std::is_base_of_v<Event, EventType>, "EventType must inherit from Event");
        return GetInstance().subscribe<EventType>(std::move(callback));
    }

    template<typename EventType>
    static void Unsubscribe(HandlerID id) {
        GetInstance().unsubscribe<EventType>(id);
    }

    template<typename EventType>
    static void Emit(const EventType& event) {
        GetInstance().emit<EventType>(event);
    }

    static void ProcessEvents() {
        GetInstance().processEvents();
    }

    template<typename EventType>
    static void Clear() {
        GetInstance().clear<EventType>();
    }

    static void ClearAll() {
        GetInstance().clearAll();
    }

    static EventBus& GetInstance();

    EventBus(const EventBus&) = delete;
    EventBus& operator=(const EventBus&) = delete;
    EventBus(EventBus&&) = delete;
    EventBus& operator=(EventBus&&) = delete;

private:
    EventBus() = default;
    ~EventBus() = default;

    struct ListenerEntry {
        HandlerID id;
        std::function<void(const Event&)> callback;
    };

    template<typename EventType>
    HandlerID subscribe(std::function<void(const EventType&)> callback) {
        std::lock_guard<std::mutex> lock(m_mutex);
        HandlerID id = m_nextID++;
        m_listeners[std::type_index(typeid(EventType))].push_back({
            id,
            [cb = std::move(callback)](const Event& e) {
                cb(static_cast<const EventType&>(e));
            }
        });
        return id;
    }

    template<typename EventType>
    void unsubscribe(HandlerID id) {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_listeners.find(std::type_index(typeid(EventType)));
        if (it == m_listeners.end()) return;
        auto& vec = it->second;
        vec.erase(std::remove_if(vec.begin(), vec.end(),
            [id](const ListenerEntry& entry) { return entry.id == id; }),
            vec.end());
    }

    template<typename EventType>
    void emit(const EventType& event) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_eventQueue.push(std::make_shared<EventType>(event));
    }

    void processEvents();

    template<typename EventType>
    void clear() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_listeners.erase(std::type_index(typeid(EventType)));
    }

    void clearAll();

    std::unordered_map<std::type_index, std::vector<ListenerEntry>> m_listeners;
    std::queue<std::shared_ptr<Event>> m_eventQueue;
    std::mutex m_mutex;
    HandlerID m_nextID = 0;
};

} // namespace Shit
