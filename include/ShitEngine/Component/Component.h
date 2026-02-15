#pragma once
#include "../Core/Config.h"

namespace Shit {
	class GameObject; // 前向声明

	/**
	 * @brief 组件基类
	 * 
	 * 任何组件都必须继承于此
	 */
	class SHIT_API Component {
		friend class GameObject; // 只能通过GameObject初始化组件

	public:
		Component() = default;
		virtual ~Component() = default;

		// 禁止拷贝和移动
		Component(const Component&) = delete;
		Component& operator=(const Component&) = delete;
		Component(Component&&) = delete;
		Component& operator=(Component&&) = delete;

	private:
		// 只允许 GameObject 设置
		inline GameObject* getOwner() { return m_owner; }
		inline void setOwner(GameObject* owner) {
			if (!m_owner) { // 判断是否存在Owner, 如果没有才设置
				m_owner = owner;
			}
		}

	protected:
		GameObject* m_owner; // 组件的拥有者
	};
}