#pragma once
#include <string>
#include <unordered_map>
#include <typeindex>
#include <memory>
#include <utility>

#include "../Core/Config.h"
#include "../Component/Component.h"
#include "../Component/Behavior.h"
#include "../Component/RendererComponent.h"
#include "../Scene/Scene.h"

namespace Shit {
	class Scene; // 前向声明

	/**
	 * @brief 游戏物体类
	 *
	 * 构造函数私有，只能通过 Scene::createGameObject 或 Scene::instantiate 创建。
	 */
	class SHIT_API GameObject final {
		friend class Scene;
	private:
		GameObject(const std::string& name);

	public:
		~GameObject() = default;

		// 禁止拷贝和移动
		GameObject(const GameObject&) = delete;
		GameObject& operator=(const GameObject&) = delete;
		GameObject(GameObject&&) = delete;
		GameObject& operator=(GameObject&&) = delete;

		void destroy(); ///< 标记为待销毁（帧末由 Scene 统一清理）

		// --- getter & setter ---
		const std::string& getName() const { return m_name; }
		const std::string& getTag() const { return m_tag; }
		Scene* getScene() const { return m_scene; }
		bool isNeedDestroy() const { return m_needDestroy; }

		void setName(const std::string& name) { m_name = name; }
		void setTag(const std::string& tag) { m_tag = tag; }  ///< 设置标签（用于分类，如 "enemy"、"player"）
		void setScene(Scene* scene);  ///< 设置所属场景（同时触发未注册组件的 onAttach）
		void setNeedDestroy(bool needDestroy) { m_needDestroy = needDestroy; }
		std::unordered_map<std::type_index, std::unique_ptr<Component>>& getComponents() { return m_components; } ///< 获取全部组件（按 type_index 索引）

		/**
		 * @brief 添加组件
		 * @tparam T 组件类型（须继承 Component）
		 * @tparam ...Args 构造参数类型
		 * @param ...args 传递给组件构造函数
		 * @return 组件指针（若已存在则返回已有的）
		 *
		T* addComponent(Args&&... args) {
			static_assert(std::is_base_of_v<Component, T>, "添加的组件必须继承自 Component！");

			auto type_index = std::type_index(typeid(T));

			if (hasComponent<T>()) { // 是否已经存在
				return getComponent<T>();
			}

			// 创建组件
			auto new_component = std::make_unique<T>(std::forward<Args>(args)...);
			T* new_component_ptr = new_component.get();
			new_component->setOwner(this);
			new_component->onCreate(); // onCreate：轻量初始化

			// 若已挂载场景则立即执行 onAttach（注册到 System）
			if (m_scene) {
				new_component->onAttach();
			}

			m_components[type_index] = std::unique_ptr<Component>(new_component.release());
			ST_CORE_TRACE("GameObject : {} 已添加 组件 {}", m_name, typeid(T).name());

			return new_component_ptr;
		}

		/**
		 * @brief 获取组件
		 * @tparam T 组件类型
		 * @return 组件裸指针
		 */
		template <typename T>
		T* getComponent() {
			static_assert(std::is_base_of_v<Component, T>, "获取的组件必须继承自 Component！");
			auto type_index = std::type_index(typeid(T));

			if (auto it = m_components.find(type_index); it != m_components.end()) {
				return static_cast<T*>(it->second.get());
			}
			return nullptr;
		}

		/**
		 * @brief 检查是否存在某个组件
		 * @tparam T 组件类型
		 * @return 是否存在组件
		 */
		template <typename T>
		bool hasComponent() {
			static_assert(std::is_base_of_v<Component, T>, "检查的组件必须继承自 Component！");

			return m_components.contains(std::type_index(typeid(T)));
		}

		/**
		 * @brief 移除某个组件
		 * @tparam T 组件类型
		 *
		 * 生命周期调用顺序：onDetach → onDestroy
		 */
		template <typename T>
		void removeComponent() {
			static_assert(std::is_base_of_v<Component, T>, "移除的组件必须继承自 Component！");
			auto type_index = std::type_index(typeid(T));

			if (auto it = m_components.find(type_index); it != m_components.end()) {
				it->second->onDetach();
				it->second->onDestroy();
				m_components.erase(it);
			}
		}

	private:
		void clean(); // 清理（只能 Scene 调用）

		std::string m_name; // 游戏物体名称
		std::string m_tag; // 标签
		Scene* m_scene = nullptr; // 所在 Scene 指针
		std::unordered_map<std::type_index, std::unique_ptr<Component>> m_components; // 挂载的组件
		bool m_needDestroy = false; // 是否需要销毁（由 Scene 负责销毁）
	};
}