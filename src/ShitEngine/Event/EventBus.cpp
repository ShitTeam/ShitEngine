#include "ShitEngine/Event/EventBus.h"
#include <algorithm>

namespace Shit {
	EventBus& EventBus::GetInstance() {
		static EventBus instance;
		return instance;
	}

	void EventBus::clearAll() {
		std::queue<std::shared_ptr<Event>> drained;
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			m_listeners.clear();
			drained.swap(m_eventQueue);  // 锁内仅做 O(1) swap，立即解锁
		}
		// drained 在作用域结束时丢弃，外部使用的资源不会被长时间持有
		(void)drained;
	}

	void EventBus::processEvents() {
		// 锁内把队列 swap 到局部拷贝，解锁后再派发；
		// 这样回调内 Subscribe/Unsubscribe/Emit 不会与派发竞争同一把锁，
		// 也避免 std::mutex 不可重入导致的死锁。
		std::queue<std::shared_ptr<Event>> local;
		std::unordered_map<std::type_index, std::vector<ListenerEntry>> snapshot;
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			local.swap(m_eventQueue);
			// 拷贝一份当前 listener 表，避免派发期间回调增删 vector 造成迭代器失效
			snapshot = m_listeners;
		}

		while (!local.empty()) {
			auto eventPtr = local.front();
			local.pop();  // 出栈后引用计数递减，回调内若再 Emit 不受影响

			auto it = snapshot.find(std::type_index(typeid(*eventPtr)));
			if (it == snapshot.end()) continue;
			// 注意：这里遍历的是快照，回调内对 m_listeners 的改动不影响本轮派发
			for (const auto& entry : it->second) {
				entry.callback(*eventPtr);
			}
		}
	}
}
