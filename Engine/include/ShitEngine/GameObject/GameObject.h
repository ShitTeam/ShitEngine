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
	template <typename T> class WeakComponentRef;  // 前向声明（组件弱引用，见文件末尾定义）

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

		void destroy(); ///< 标记为待销毁（帧末由 Scene 统一清理）；级联标记所有子物体

		// --- 父子关系 ---
		/**
		 * @brief 设置父物体
		 *
		 * 自动从旧父物体的子列表中移除自己，并加入新父物体的子列表。
		 * 传入 nullptr 即解除当前父子关系。跨 Scene 的父子关系将被拒绝并记 WARN。
		 * @param parent 新父物体（可为 nullptr）
		 */
		void setParent(GameObject* parent);

		GameObject* getParent() const { return m_parent; }                        ///< 获取父物体
		const std::vector<GameObject*>& getChildren() const { return m_children; } ///< 获取子物体列表

		/// @brief 添加子物体（等价于 child->setParent(this)）
		void addChild(GameObject* child) { if (child) child->setParent(this); }

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

		/// @brief 只读遍历全部组件（供系统补扫等使用，避免暴露内部可变 map）
		template <typename Fn>
		void forEachComponent(Fn&& fn) {
			for (auto& [type, comp] : m_components) {
				if (comp) fn(comp.get());
			}
		}

		/// @brief 创建指向本对象上 T 组件的弱引用（组件被移除/对象销毁后自动失效）
		template <typename T>
		WeakComponentRef<T> getWeakRef() {
			return WeakComponentRef<T>(this, m_lifetime);
		}

		/**
		 * @brief 添加组件
		 * @tparam T 组件类型（须继承 Component）
		 * @tparam Args 构造参数类型
		 * @param args 传递给组件构造函数
		 * @return 组件指针（若已存在则返回已有的）
		 */

		/**
		 * @brief 动态添加组件实例（Prefab 实例化等反射场景用，所有权转移给 GameObject）
		 * @param component 已通过反射工厂创建的组件实例
		 * @return 添加后的组件指针（若已存在同类型，则丢弃传入实例并返回已有的）
		 */
		Component* addComponentInstance(Component* component);

		template <typename T, typename... Args>
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

			// 先插入容器，再触发生命周期回调。若 onAttach/onCreate 内销毁了 owner
			//（如 removeGameObject 立即清理），对已释放对象写 map 会堆破坏；先入表则
			// 回调内可以安全地把自己移除，clean() 也能遍历到它。
			m_components[type_index] = std::unique_ptr<Component>(new_component.release());

			// 捕获回调可能用到的数据：回调内可能销毁 owner（this），之后不能再触碰 this
			Scene* scene = m_scene;
			std::weak_ptr<void> lifetime = m_lifetime;
			const std::string goName = m_name;

			new_component_ptr->onCreate(); // onCreate：轻量初始化
			if (lifetime.expired()) return nullptr;   // 回调销毁了 owner → 组件已随 clean() 释放

			// 若已挂载场景则立即执行 onAttach（注册到 System）。
			// 注册状态由组件自身的 onAttach 维护（基类默认置 true），此处不强制置位，
			// 以便"组件先加、驱动系统后注册"时能被 System::init 补挂。
			if (scene && !new_component_ptr->isRegistered()) {
				new_component_ptr->onAttach();
			}
			if (lifetime.expired()) return nullptr;   // onAttach 销毁了 owner

			ST_CORE_TRACE("GameObject : {} 已添加 组件 {}", goName, typeid(T).name());

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
				// 先从容器取出再调回调，避免回调内再次 removeComponent 造成迭代器失效/重复回调
				auto comp = std::move(it->second);
				m_components.erase(it);
				comp->onDetach();
				comp->onDestroy();
			}
		}

	private:
		void clean(); // 清理（只能 Scene 调用）
		void removeFromParentChildren(); // 从当前父物体的子列表中移除自己

		std::string m_name; // 游戏物体名称
		std::string m_tag; // 标签
		Scene* m_scene = nullptr; // 所在 Scene 指针
		std::unordered_map<std::type_index, std::unique_ptr<Component>> m_components; // 挂载的组件
		bool m_needDestroy = false; // 是否需要销毁（由 Scene 负责销毁）

		GameObject* m_parent = nullptr; // 父物体（裸指针，所有权归 Scene）
		std::vector<GameObject*> m_children; // 子物体列表（裸指针，仅表达层级）

		// 生命周期令牌：本对象随 unique_ptr 销毁时，所有持 weak_ptr 的 WeakComponentRef 自动失效，
		// 避免 WeakComponentRef 在 owner 已被销毁后解引用悬垂的 GameObject 指针。
		std::shared_ptr<void> m_lifetime = std::make_shared<int>(0);
	};

	/**
	 * @brief 组件弱引用
	 *
	 * 安全持有组件引用而不导致悬垂：不存组件指针，只存 owner + 生命周期令牌 + 模板类型，
	 * 访问时通过 owner->getComponent<T>() 现查。组件被 removeComponent 移除、或所属
	 * GameObject 被销毁后，get() 返回 nullptr（逻辑已死可检测），不会 use-after-free。
	 *
	 * 用法：
	 *   auto ref = go->getWeakRef<Shit::UIText>();
	 *   button->setOnClick([ref]() {
	 *       if (Shit::UIText* t = ref.get()) t->setText("...");  // 已移除则安全跳过
	 *   });
	 *
	 * 注意：这是"弱引用"不是"强引用"——不能阻止组件销毁，也不延长其生命周期。
	 * 若组件必须长期存活，属于生命周期设计问题而非引用问题。
	 */
	template <typename T>
	class WeakComponentRef {
	public:
		WeakComponentRef() = default;
		explicit WeakComponentRef(GameObject* owner, const std::weak_ptr<void>& lifetime)
			: m_owner(owner), m_lifetime(lifetime) {}

		/// @brief 获取组件（组件已移除或对象已销毁则返回 nullptr）
		T* get() const {
			if (!m_owner || m_lifetime.expired()) return nullptr;
			return m_owner->template getComponent<T>();
		}

		/// @brief 组件当前是否有效（onAttach 后、removeComponent 前，且 owner 未被销毁）
		bool valid() const { return get() != nullptr; }

		/// @brief 便捷解引用（调用前建议先 valid() 检查或直接解引用判空）
		T* operator->() const { return get(); }
		explicit operator bool() const { return valid(); }

		/// @brief 获取所属 GameObject（可能已被销毁，调用方需自行保证安全）
		GameObject* getOwner() const { return m_owner; }

	private:
		GameObject* m_owner = nullptr;
		std::weak_ptr<void> m_lifetime;
	};
}