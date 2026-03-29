#pragma once
#include "../Core/Config.h"
#include "../Core/pch.h"
#include "../Component/Component.h"
#include "../Component/Behavior.h"
#include "../Component/RendererComponent.h"
#include "../Scene/Scene.h"
#include <typeindex>

namespace Shit {
	class Scene; // 前向声明
	
	/**
	 * @brief 游戏物体类
	 */
	class SHIT_API GameObject final {
		friend class Scene;
	public:
		GameObject(const std::string& name);
		~GameObject() = default;

		// 禁止拷贝和移动
		GameObject(const GameObject&) = delete;
		GameObject& operator=(const GameObject&) = delete;
		GameObject(GameObject&&) = delete;
		GameObject& operator=(GameObject&&) = delete;

		void update(); // 更新
		void destroy(); // 销毁

		// --- getter & setter ---
		inline std::string& getName() { return m_name; }
		inline std::string& getTag() { return m_tag; }
		inline Scene* getScene() { return m_scene; }
		inline bool isNeedDestroy() const { return m_needDestroy; }

		inline void setName(std::string& name) { m_name = name; }
		inline void setTag(std::string& tag) { m_tag = tag; }
		inline void setScene(Scene* scene) { m_scene = scene; }
		inline void setNeedDestroy(bool needDestroy) { m_needDestroy = needDestroy; }

		/**
		 * @brief 添加组件
		 * @tparam T 组件类型
		 * @tparam ...Args 传递的参数
		 * @param ...args 组件构造函数参数
		 * @return 组件裸指针
		 */
		template <typename T, typename... Args>
		T* addComponent(Args&&... args) {
			// 检查 T 是不是继承自 Component
			static_assert(std::is_base_of_v<Component, T>, "添加的组件必须继承自 Component！");

			std::type_index type_index = std::type_index(typeid(T));

			if (hasComponent<T>()) { // 是否已经存在
				return getComponent<T>();
			}

			// 创建组件
			auto new_component = std::make_unique<T>(std::forward<Args>(args)...);
			T* new_component_ptr = new_component.get();
			new_component->setOwner(this);
			new_component->onAttach(); // 执行 onAttach 生命周期

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
		 */
		template <typename T>
		void removeComponent() {
			static_assert(std::is_base_of_v<Component, T>, "移除的组件必须继承自 Component！");
			auto type_index = std::type_index(typeid(T));

			if (auto it = m_components.find(type_index); it != m_components.end()) {

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
		std::vector<Behavior*> m_behaviors; // 挂载的行为组件
		bool m_needDestroy = false; // 是否需要销毁（由 Scene 负责销毁）
	};
}