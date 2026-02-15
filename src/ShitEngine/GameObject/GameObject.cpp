#include "ShitEngine/GameObject/GameObject.h"

namespace Shit {
	Shit::GameObject::GameObject(const std::string& name) : m_name(name)
	{
		ST_CORE_TRACE("游戏物体: {} 被创建!", m_name);
	}
}