#pragma once
#include "../Core/Config.h"

namespace Shit {
	class GameObject; // 前向声明

	/**
	 * @brief 组件基类
	 *
	 * 所有组件必须继承此类。
	 *
	 * 生命周期（按调用顺序）：
	 *   onCreate  — 组件刚被构造、已绑定 owner，尚未挂载到场景
	 *   onAttach  — 组件所属 GameObject 进入活动场景时
	 *   onDetach  — 组件即将从场景中移除时（在 onDestroy 前调用）
	 *   onDestroy — 组件被销毁时
	 */
	class SHIT_API Component {
		friend class GameObject; // 只能通过GameObject初始化组件

	public:
		Component();
		virtual ~Component() = default;

		virtual void onCreate() {}   // 组件创建时（有 owner，但可能尚未挂到场景）
		virtual void onAttach() {}   // 组件挂载到活动场景时
		virtual void onDetach() {}   // 组件从场景卸下时
		virtual void onDestroy() {}  // 组件销毁时

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