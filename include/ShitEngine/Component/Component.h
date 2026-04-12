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
		Component();
		virtual ~Component() = default;

		virtual void onAttach() {} // 挂载时
		virtual void onDestroy() {} // 销毁时

		// 禁止拷贝和移动
		Component(const Component&) = delete;
		Component& operator=(const Component&) = delete;
		Component(Component&&) = delete;
		Component& operator=(Component&&) = delete;

		GameObject* getOwner() const { return m_owner; }
		bool isRegistered() const { return m_isRegistered; }

	private:
		// 只允许 GameObject 设置
		inline void setOwner(GameObject* owner) { m_owner = owner; }

	protected:
		GameObject* m_owner = nullptr; // 组件的拥有者

		bool m_isRegistered = false;
	};
}