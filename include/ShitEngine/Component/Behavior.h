#pragma once
#include "Component.h"

namespace Shit {
	class GameObject; // 前向声明

	/**
	 * @brief 行为基类
	 *
	 * 继承自 Component，拥有完整生命周期：
	 *   onCreate → onAttach → onStart → (每帧)onUpdate → onDetach → onDestroy
	 */
	class SHIT_API Behavior : public Component {
		friend class GameObject;
	public:
		Behavior() = default;
		~Behavior() override = default;

		// --- 生命周期 ---
		void onCreate() override;
		void onAttach() override;
		virtual void onStart();		  // 组件更新前执行一次
		virtual void onUpdate();	  // 每帧执行
		void onDetach() override;
		void onDestroy() override;

		bool isStarted() const { return m_isStarted; }
		void setStarted(bool isStarted) { m_isStarted = isStarted; }

	protected:
		bool m_isStarted = false; // 是否运行过 onStart 函数
	};
}