#include "ShitEngine/GameObject/GameObject.h"

namespace Shit {
	GameObject::GameObject(const std::string& name) : m_name(name)
	{
		ST_CORE_TRACE("游戏物体: {} 被创建!", m_name);
	}

	void GameObject::destroy() {
		m_needDestroy = true;
	}

	void GameObject::setScene(Scene* scene) {
		m_scene = scene;

		// 进入场景时：对尚未注册的组件执行 onAttach
		if (scene) {
			for (auto& [type, comp] : m_components) {
				if (!comp->isRegistered()) {
					comp->onAttach();
				}
			}
		}
	}

	void GameObject::clean()
	{
		for (auto& [type, comp] : m_components) {
			comp->onDetach();
			comp->onDestroy();
		}
		m_components.clear();
	}
}
