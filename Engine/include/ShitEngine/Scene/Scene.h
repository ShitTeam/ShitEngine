#pragma once
#include <cstdint>
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
	class Component;
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
	 *   scene->init();                    // 注册默认 BehaviorSystem + RenderSystem + UIRenderSystem
	 *   auto* player = scene->createGameObject("player");
	 *   SceneManager::LoadScene(std::move(scene));
	 */
	class SHIT_API Scene {
		friend class GameObject; ///< GameObject 结构变更（组件增删/改名/改父）时通知 bumpGeneration
	public:
		explicit Scene(const std::string& name);
		~Scene();

		// 禁止拷贝和移动构造
		Scene(const Scene&) = delete;
		Scene& operator=(const Scene&) = delete;
		Scene(Scene&&) = delete;
		Scene& operator=(Scene&&) = delete;

		virtual void init();    ///< 注册默认系统（BehaviorSystem + RenderSystem + UIRenderSystem），幂等
		void update();           ///< 更新所有 System + 处理延迟操作
		virtual void destroy(); ///< 销毁所有对象与系统

		/// @brief 是否已注册任何 System（SceneManager 用于自动初始化，防止漏调 init() 导致空场景）
		bool hasSystems() const { return !m_systems.empty(); }

		void addGameObject(std::unique_ptr<GameObject>&& gameObject);  ///< 延迟添加 GameObject（帧末生效）
		GameObject* createGameObject(const std::string& name);          ///< 创建并添加 GameObject
		GameObject* instantiate(const Prefab& prefab, const std::string& name = ""); ///< 从预制体实例化
		void removeGameObject(GameObject* gameObject);                  ///< 按指针标记销毁
		void removeGameObjectByName(const std::string& name);           ///< 按名称标记销毁

		/// @brief 组件挂载广播（组件 onAttach 调用）。分发给可处理的系统，返回是否有系统认领。
		bool registerComponent(Component* component);
		/// @brief 组件卸下广播（组件 onDetach 调用）。
		void unregisterComponent(Component* component);

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

			// 先入表再 init()：System::init() 会补扫场景中未注册的组件并广播 registerComponent。
			// 若尚未加入 m_systems，新系统将收不到已存在组件的 onComponentAttached，
			// "组件先加、驱动系统后注册"的补挂机制会失效。
			m_systemsMap[type_index] = std::unique_ptr<System>(new_system.release());
			m_systems.push_back(new_system_ptr);
			m_isSystemsNeedSort = true;

			new_system_ptr->init();

			ST_CORE_TRACE("Scene : {} 已添加系统 {}", m_name, typeid(T).name());

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

			// ── 按字符串名称的系统管理（供编辑器运行时使用） ──

			/// @brief 按类型名注册系统（通过反射 Factory 创建实例）。幂等（已有则返回现有）。
			/// 失败（类型未注册/无工厂/非 System 派生）返回 nullptr 并 WARN。
			System* registerSystem(const std::string& typeName);

			/// @brief 按类型名查询系统（返回 nullptr 表示未注册）
			System* getSystem(const std::string& typeName) const;

			/// @brief 按类型名判断系统是否已注册
			bool hasSystem(const std::string& typeName) const;

			/// @brief 按类型名延迟移除系统
			void unregisterSystem(const std::string& typeName);

			/// @brief 返回当前已注册系统的类型名列表（按优先级排序）
			std::vector<std::string> getRegisteredSystemTypeNames() const;

			/// @brief 调整系统优先级（即时生效，下一帧 update 前重排序）
			void setSystemPriority(const std::string& typeName, int priority);

			/// @brief 卸载指定来源注册的所有系统（插件卸载前调用，防止卸载 DLL 后 vtable 悬垂）
			void unregisterSystemsBySource(const std::string& source);

			/// @brief 立即处理待移除系统队列（热重载卸载 DLL 前调用，确保 vtable 悬垂前销毁系统）
			void flushPendingSystemRemovals();

		// --- getter & setter ---
		const std::string& getName() const { return m_name; }
		std::vector<std::unique_ptr<GameObject>>& getGameObjects() { return m_gameObjects; }

		void setName(const std::string& name) { m_name = name; }

		/// @brief 场景结构代数：任何对象增删/组件增删/改名/改父都会递增。
		/// 编辑器据此判断"场景内容是否变化"，及时重建场景树、校验选中态，
		/// 避免播放中对象被游戏逻辑销毁后仍持有悬垂指针。
		uint64_t getGeneration() const { return m_generation; }

		/// @brief 对象是否仍在当前场景容器中（地址比较，供编辑器校验旧选中指针）
		bool containsGameObject(const GameObject* gameObject) const;

		/// @brief 按持久 UUID 查找组件（ComponentRef<T> 懒解析 / 编辑器拖拽引用共用）
		/// 组件已移除或对象已销毁后返回 nullptr（UUID 索引随组件生命周期维护）。
		Component* componentByUuid(uint64_t uuid) const { return m_uuidMap.count(uuid) ? m_uuidMap.at(uuid) : nullptr; }

	private:
		void processPendingAdditions(); // 处理延迟添加
		void processPendingRemoveSystems();
		void bumpGeneration() { ++m_generation; } ///< 结构变更标记（GameObject 经 friend 调用）

		/// @brief 把组件 UUID 纳入索引（幂等；冲突（同 uuid 属另一组件）时重发 uuid 并重试）
		/// 供 registerComponent / GameObject 挂载组件时调用，覆盖不主动注册系统的组件。
		void indexComponentUuid(Component* component);
		/// @brief 从索引移除组件（幂等；仅当索引项确为本组件时擦除，防误删）
		void unindexComponentUuid(Component* component);

		std::string m_name; // 场景名称
		std::vector<std::unique_ptr<GameObject>> m_gameObjects; // 游戏物体
		std::vector<std::unique_ptr<GameObject>> m_pendingAdditions; // 延迟添加
		uint64_t m_generation = 0; ///< 结构代数（编辑器同步用）
		std::unordered_map<uint64_t, Component*> m_uuidMap; ///< 组件持久 ID → 组件（引用字段寻址）

		std::unordered_map<std::type_index, std::unique_ptr<System>> m_systemsMap; // 注册的系统
		std::vector<System*> m_systems; // 缓存的系统
		std::vector<std::type_index> m_pendingRemoveSystems;
		bool m_isSystemsNeedSort = false;
		bool m_isInited = false;  ///< init() 是否已执行（幂等守卫）
	};
}
