#pragma once
#include "../Core/Config.h"
#include "../Core/pch.h"
#include "../Component/Component.h"
#include "../Component/Behavior.h"
#include <typeindex>

namespace Shit {
	
	/**
	 * @brief 游戏物体类
	 */
	class SHIT_API GameObject final {
	public:
		GameObject(const std::string& name);
		~GameObject() = default;

		// 禁止拷贝和移动
		GameObject(const GameObject&) = delete;
		GameObject& operator=(const GameObject&) = delete;
		GameObject(GameObject&&) = delete;
		GameObject& operator=(GameObject&&) = delete;

		// --- getter & setter ---
		inline std::string& getName() { return m_name; }
		inline void setName(std::string& name) { m_name = name; }

		/**
		 * @brief 添加组件
		 * @tparam T 组件类型
		 * @tparam ...Args 
		 * @param ...args 组件构造函数参数
		 * @return 组件裸指针
		 */
		template <typename T, typename... Args>
		T* addComponent(Args&&... args) {
			// 检查 T 是不是继承自 Component
			static_assert(std::is_base_of<Component, T>::value, "添加的组件必须继承自 Component！");

			std::type_index type_index = std::type_index(typeid(T));

			if (hasComponent<T>()) { // 是否已经存在
				return getComponent<T>();
			}

			// 创建组件
			auto new_component = std::make_unique<T>(std::forward<Args>(args)...);
			T* new_component_ptr = new_component.get();
			new_component->setOwner(this);


			Behavior* new_behavior = dynamic_cast<Behavior*>(new_component_ptr);
			if (new_behavior) { // 如果不是 Behavior，则为nullptr
				m_behaviors.push_back(new_behavior);
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
			static_assert(std::is_base_of<Component, T>::value, "获取的组件必须继承自 Component！");
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
			static_assert(std::is_base_of<Component, T>::value, "检查的组件必须继承自 Component！");

			return m_components.contains(std::type_index(typeid(T)));
		}

		/**
		 * @brief 移除某个组件
		 * @tparam T 组件类型
		 */
		template <typename T>
		void removeComponent() {
			static_assert(std::is_base_of<Component, T>::value, "移除的组件必须继承自 Component！");
			auto type_index = std::type_index(typeid(T));

			if (auto it = m_components.find(type_index); it != m_components.end()) {

				if (Behavior* behavior = dynamic_cast<Behavior*>(it->second.get())) {
					// 销毁
					behavior->onDestroy();

					// 从更新列表中移除原始指针
					// std::remove可以将要删除的behavior移动到最后，再配合erase便可以高效删除
					// 每日查资料（bushi）
					m_behaviors.erase(
						std::remove(m_behaviors.begin(), m_behaviors.end(), behavior),
						m_behaviors.end()
					);
				}

				m_components.erase(it);
			}
		}

	private:
		std::string m_name; // 游戏物体名称
		std::unordered_map<std::type_index, std::unique_ptr<Component>> m_components; // 挂载的组件
		std::vector<Behavior*> m_behaviors; // 挂载的行为组件
	};
}