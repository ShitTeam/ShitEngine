#pragma once
#include <typeindex>
#include <vector>
#include <memory>
#include <unordered_map>
#include <string>

#include "../Core/Core.h"
#include "ShitEngine/Core/Log.h"
#include "ShitEngine/System/System.h"

namespace Shit {
	class System;
	// 前向声明
	class CameraComponent;
	class SceneManager;
	class GameObject;
	class Prefab;
	class Behavior;
	class RendererComponent;

	/**
	 * @brief 场景类
	 *
	 * 场景是游戏世界的容器，管理所有 GameObject、System 的生命周期。
	 * 通过 SceneManager 的栈机制实现场景切换（主菜单→游戏→暂停）。
	 *
	 * 使用方式：
	 *   auto scene = std::make_unique<Scene>("level1");
	 *   scene->init();                    // 注册默认 BehaviorSystem + RenderSystem
	 *   auto* player = scene->createGameObject("player");
	 *   SceneManager::PushScene(std::move(scene));
	 */
	class SHIT_API Scene {
	public:
		explicit Scene(const std::string& name);
		~Scene();

		// 禁止拷贝和移动构造
		Scene(const Scene&) = delete;
		Scene& operator=(const Scene&) = delete;
		Scene(Scene&&) = delete;
		Scene& operator=(Scene&&) = delete;

		virtual void init();    ///< 注册默认系统（BehaviorSystem + RenderSystem）
		void update();           ///< 更新所有 System + 处理延迟操作
		virtual void destroy(); ///< 销毁所有对象与系统

		void addGameObject(std::unique_ptr<GameObject>&& gameObject);  ///< 延迟添加 GameObject（帧末生效）
		GameObject* createGameObject(const std::string& name);          ///< 创建并添加 GameObject
		GameObject* instantiate(const Prefab& prefab, const std::string& name = ""); ///< 从预制体实例化
		void removeGameObject(GameObject* gameObject);                  ///< 按指针标记销毁
		void removeGameObjectByName(const std::string& name);           ///< 按名称标记销毁

		template <typename T>
		T* registerSystem() {
			static_assert(std::is_base_of_v<System, T>, "必须继承自 System 基类");

			auto type_index = std::type_index(typeid(T));

			if (hasSystem<T>()) {
				return getSystem<T>();
			}

			auto new_system = std::make_unique<T>();
			T* new_system_ptr = new_system.get(); // 返回的指针

			new_system_ptr->setScene(this);
			new_system_ptr->init();

			m_systemsMap[type_index] = std::unique_ptr<System>(new_system.release());
			m_systems.push_back(new_system_ptr);
			ST_CORE_TRACE("Scene : {} 已添加系统 {}", m_name, typeid(T).name());

			m_isSystemsNeedSort = true;

			return new_system_ptr;
		}

		template <typename T>
		void unregisterSystem() {
			static_assert(std::is_base_of_v<System, T>, "必须继承自 System 基类");

			m_pendingRemoveSystems.push_back(std::type_index(typeid(T))); // 延迟移除
		}

		template <typename T>
		T* getSystem() {
			static_assert(std::is_base_of_v<System, T>, "必须继承自 System 基类");
			auto type_index = std::type_index(typeid(T));

			if (auto it = m_systemsMap.find(type_index); it != m_systemsMap.end()) {
				return static_cast<T*>(it->second.get());
			}
			return nullptr;
		}

		template <typename T>
		bool hasSystem() {
			static_assert(std::is_base_of_v<System, T>, "必须继承自 System 基类");

			return m_systemsMap.contains(std::type_index(typeid(T)));
		}

		// --- getter & setter ---
		const std::string& getName() const { return m_name; }
		std::vector<std::unique_ptr<GameObject>>& getGameObjects() { return m_gameObjects; }

		void setName(const std::string& name) { m_name = name; }
	private:
		void processPendingAdditions(); // 处理延迟添加
		void processPendingRemoveSystems();

		std::string m_name; // 场景名称
		std::vector<std::unique_ptr<GameObject>> m_gameObjects; // 游戏物体
		std::vector<std::unique_ptr<GameObject>> m_pendingAdditions; // 延迟添加

		std::unordered_map<std::type_index, std::unique_ptr<System>> m_systemsMap; // 注册的系统
		std::vector<System*> m_systems; // 缓存的系统
		std::vector<std::type_index> m_pendingRemoveSystems;
		bool m_isSystemsNeedSort = false;
	};
}
