#include "ShitEngine/GameObject/GameObject.h"

namespace Shit {
	GameObject::GameObject(const std::string& name) : m_name(name)
	{
		ST_CORE_TRACE("游戏物体: {} 被创建!", m_name);
	}

	void GameObject::destroy() {
		m_needDestroy = true;
	}

	void GameObject::clean()
	{
		for (auto& comp : m_components) {
			comp.second->onDestroy();
		}
		m_components.clear();
	}
}