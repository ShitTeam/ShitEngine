#include "ShitEngine/Core/pch.h"
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

	Component* GameObject::addComponentInstance(Component* component) {
		if (!component) return nullptr;
		auto type_index = std::type_index(typeid(*component));

		// 已存在同类型：丢弃传入实例，返回已有的
		if (auto it = m_components.find(type_index); it != m_components.end()) {
			ST_CORE_WARN("GameObject : {} 已存在组件 {}，丢弃传入实例", m_name, typeid(*component).name());
			delete component;
			return it->second.get();
		}

		component->setOwner(this);
		m_components[type_index] = std::unique_ptr<Component>(component);

		// 捕获回调可能用到的数据：回调内可能销毁 owner（this），之后不能再触碰 this
		Scene* scene = m_scene;
		std::weak_ptr<void> lifetime = m_lifetime;
		const std::string goName = m_name;

		component->onCreate();
		if (lifetime.expired()) return nullptr;   // 回调销毁了 owner → 组件已随 clean() 释放

		// 若已挂载场景则立即执行 onAttach（注册到 System）
		if (scene && !component->isRegistered()) {
			component->onAttach();
		}
		if (lifetime.expired()) return nullptr;   // onAttach 销毁了 owner

		return component;
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

		// 检查间接循环引用（如 A -> B -> A）
		for (GameObject* ancestor = parent; ancestor; ancestor = ancestor->m_parent) {
			if (ancestor == this) {
				ST_CORE_WARN("GameObject {} 设置父物体将造成循环引用，已拒绝", m_name);
				return;
			}
		}

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
		// 但需清空子物体的父指针，避免悬空引用
		for (auto* child : m_children) {
			if (child) child->m_parent = nullptr;
		}
		m_children.clear();

		// 逐个取出组件再调生命周期回调，避免回调内 removeComponent 修改 map 造成迭代器失效
		while (!m_components.empty()) {
			auto it = m_components.begin();
			auto comp = std::move(it->second);
			m_components.erase(it);
			comp->onDetach();
			comp->onDestroy();
		}
	}
}