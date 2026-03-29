#include "ShitEngine/GameObject/GameObject.h"

namespace Shit {
	GameObject::GameObject(const std::string& name) : m_name(name)
	{
		ST_CORE_TRACE("游戏物体: {} 被创建!", m_name);
	}

	void GameObject::update() {
		for (auto& b : m_behaviors) {
			if (!b->isStarted()) {
				b->onStart(); // 执行 onStart 生命周期
				b->setStarted(true);
			}
			b->onUpdate();
		}
	}

	void GameObject::destroy() {
		m_needDestroy = true;
	}

	void GameObject::clean()
	{
		for (auto& b : m_behaviors) {
			b->onDestroy();
		}
		m_behaviors.clear();
		m_components.clear();
	}
}