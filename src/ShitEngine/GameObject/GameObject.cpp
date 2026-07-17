#include "ShitEngine/GameObject/GameObject.h"
#include "ShitEngine/Scene/Scene.h"
#include "ShitEngine/Core/Log.h"

#include <algorithm>

namespace Shit {
	GameObject::GameObject(const std::string& name) : m_name(name)
	{
		ST_CORE_TRACE("游戏物体: {} 被创建!", m_name);
	}

	void GameObject::destroy() {
		m_needDestroy = true;

		// 级联：所有子物体一并标记销毁
		for (auto* child : m_children) {
			if (child && !child->m_needDestroy) {
				child->destroy();
			}
		}
	}

	void GameObject::setParent(GameObject* parent) {
		// 跨 Scene 父子关系拒绝（避免 Scene 销毁后悬空）
		if (parent) {
			if (parent->m_scene != m_scene) {
				ST_CORE_WARN("GameObject {} 试图设置不同场景的父物体 {}，已拒绝", m_name, parent->m_name);
				return;
			}
		}

		// 自身做父节点无意义
		if (parent == this) return;

		// 从旧父物体的子列表移除自己
		removeFromParentChildren();

		// 挂到新父物体
		m_parent = parent;
		if (parent) {
			parent->m_children.push_back(this);
		}
	}

	void GameObject::removeFromParentChildren() {
		if (!m_parent) return;

		auto& siblings = m_parent->m_children;
		auto it = std::find(siblings.begin(), siblings.end(), this);
		if (it != siblings.end()) {
			siblings.erase(it);
		}
		m_parent = nullptr;
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

			// 级联：所有子物体也设置场景
			for (auto* child : m_children) {
				if (child && child->m_scene != scene) {
					child->setScene(scene);
				}
			}
		}
	}

	void GameObject::clean()
	{
		// 先与父物体解绑，避免父物体子列表残留悬空指针
		removeFromParentChildren();

		// 子物体由各自在 Scene 的销毁流程回收，这里不主动 clean 子物体，
		// 仅清空自身的镜像列表（这些指针随后也会被 Scene clean 掉）
		m_children.clear();

		for (auto& [type, comp] : m_components) {
			comp->onDetach();
			comp->onDestroy();
		}
		m_components.clear();
	}
}