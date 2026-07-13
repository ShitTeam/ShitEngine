#include "ShitEngine/Event/EventBus.h"
#include <algorithm>

namespace Shit {
	EventBus& EventBus::GetInstance() {
		static EventBus instance;
		return instance;
	}

	void EventBus::clearAll() {
		std::lock_guard<std::mutex> lock(m_mutex);
		m_listeners.clear();
		while (!m_eventQueue.empty()) m_eventQueue.pop();
	}

	void EventBus::processEvents() {
		std::lock_guard<std::mutex> lock(m_mutex);
		while (!m_eventQueue.empty()) {
			auto eventPtr = m_eventQueue.front();
			m_eventQueue.pop();

			auto it = m_listeners.find(std::type_index(typeid(*eventPtr)));
			if (it == m_listeners.end()) continue;
			for (const auto& entry : it->second) {
				entry.callback(*eventPtr);
			}
		}
	}
}
