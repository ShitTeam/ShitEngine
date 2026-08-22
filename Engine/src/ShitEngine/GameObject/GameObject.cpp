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
		if (m_scene) m_scene->bumpGeneration();   // 组件结构变更：通知场景代数递增

		// 捕获回调可能用到的数据：回调内可能销毁 owner（this），之后不能再触碰 this
		Scene* scene = m_scene;
		std::weak_ptr<void> lifetime = m_lifetime;
		const std::string goName = m_name;

		component->onCreate();
		if (lifetime.expired()) return nullptr;   // 回调销毁了 owner → 组件已随 clean() 释放

		// onCreate 回调可能 removeComponent 移除本组件（owner 未销毁时 lifetime 检查不生效），
		// 先确认组件仍归属本对象，避免后续 isRegistered/onAttach 解引用已析构实例
		{
			auto it = m_components.find(type_index);
			if (it == m_components.end() || it->second.get() != component) return nullptr;
		}

		// 若已挂载场景则立即执行 onAttach（注册到 System）
		if (scene && !component->isRegistered()) {
			component->onAttach();
		}
		if (lifetime.expired()) return nullptr;   // onAttach 销毁了 owner

		// P20: 组件 UUID 入场景索引（引用字段寻址；幂等）
		// 防御：onCreate/onAttach 回调可能已移除本组件（实例已析构），确认仍归属再索引
		if (scene) {
			auto it = m_components.find(type_index);
			if (it != m_components.end() && it->second.get() == component) {
				scene->indexComponentUuid(component);
			}
		}

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

		// 层级结构变更：通知场景代数递增（编辑器场景树据此刷新父子关系）
		if (m_scene) m_scene->bumpGeneration();
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
			// 快照遍历：onAttach 回调可能 removeComponent / 销毁 owner（此对象），
			// 在回调前确认组件仍属于本对象、owner 仍存活，避免对已释放内存调用。
			std::vector<Component*> comps;
			comps.reserve(m_components.size());
			for (auto& [type, comp] : m_components) {
				if (comp && !comp->isRegistered()) comps.push_back(comp.get());
			}

			const std::weak_ptr<void> lifetime = m_lifetime;
			for (auto* comp : comps) {
				if (lifetime.expired()) return;   // 回调销毁了 owner → 组件已随 clean() 释放

				// 回调期间组件可能已被 removeComponent：不再属于本对象则跳过
				bool owned = false;
				for (auto& [type, c] : m_components) {
					if (c.get() == comp) { owned = true; break; }
				}
				if (!owned || comp->isRegistered()) continue;

				comp->onAttach();
			}

			// P20: 全量补 UUID 索引（覆盖不主动注册系统的组件，如 TransformComponent；
			// 幂等——已索引过的组件 try_emplace 命中自身直接返回）
			for (auto& [type, comp] : m_components) {
				if (comp) scene->indexComponentUuid(comp.get());
			}

			// 级联：所有子物体也设置场景。
			// 快照遍历：深层 onAttach（用户可覆写）内 setParent/addChild 可能改动
			// m_children，range-for 迭代器会因 vector 重分配失效（AGENTS.md 约定）
			const std::vector<GameObject*> childrenSnapshot = m_children;
			for (auto* child : childrenSnapshot) {
				if (child && child->m_scene != scene && child->getParent() == this) {
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
			// P20: 组件 UUID 出索引（引用字段随即失效；幂等）
			if (m_scene) m_scene->unindexComponentUuid(comp.get());
			comp->onDetach();
			comp->onDestroy();
		}
	}
}