#pragma once
#include "Component.h"

namespace Shit {
	class GameObject;

	/**
	 * @brief 行为基类 —— 用于编写自定义游戏逻辑
	 *
	 * 继承自 Component，扩展出 onStart / onUpdate 两个阶段：
	 *   onCreate → onAttach → onStart → (每帧)onUpdate → onDetach → onDestroy
	 *
	 * onStart  在首次 update 前执行一次，适用于缓存指针。
	 * onUpdate 每帧执行，适用于输入、移动、碰撞检测等。
	 *
	 * 碰撞回调（onCollisionEnter/Stay/Exit）由 PhysicsSystem2D 在物理步进后驱动，
	 * 仅已启动（onStart 已执行）的行为收到，参数为碰撞对方的 GameObject。
	 *
	 * 挂载后由 BehaviorSystem 自动驱动，无需手动调用。
	 */
	class SHIT_API SHIT_REFLECT(BlackList) Behavior : public Component {
		friend class GameObject;
		SHIT_REFLECT_BODY(Behavior)
	public:
		Behavior() = default;
		~Behavior() override = default;

		// --- 生命周期 ---
		void onCreate() override;
		void onAttach() override;
		virtual void onStart();         ///< 首次 update 前执行一次
		virtual void onUpdate();        ///< 每帧执行

		// --- 碰撞回调（由 PhysicsSystem2D 驱动；仅 onStart 已执行的行为可收到） ---
		virtual void onCollisionEnter(GameObject* other); ///< 与 other 开始接触（每对象对一次）
		virtual void onCollisionStay(GameObject* other);  ///< 与 other 持续接触（每帧一次）
		virtual void onCollisionExit(GameObject* other);  ///< 与 other 结束接触

		void onDetach() override;
		void onDestroy() override;

		bool isStarted() const { return m_isStarted; }         ///< onStart 是否已执行过
		void setStarted(bool isStarted) { m_isStarted = isStarted; }

	protected:
		SHIT_META(({.displayName = "Started", .tooltip = "onStart 是否已执行过", .readOnly = true}))
		bool m_isStarted = false;
	};
}