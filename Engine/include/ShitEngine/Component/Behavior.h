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
	 * 挂载后由 BehaviorSystem 自动驱动，无需手动调用。
	 */
	class SHIT_API Behavior : public Component {
		friend class GameObject;
	public:
		Behavior() = default;
		~Behavior() override = default;

		// --- 生命周期 ---
		void onCreate() override;
		void onAttach() override;
		virtual void onStart();         ///< 首次 update 前执行一次
		virtual void onUpdate();        ///< 每帧执行
		void onDetach() override;
		void onDestroy() override;

		bool isStarted() const { return m_isStarted; }         ///< onStart 是否已执行过
		void setStarted(bool isStarted) { m_isStarted = isStarted; }

	protected:
		bool m_isStarted = false;
	};
}