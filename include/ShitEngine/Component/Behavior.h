#pragma once
#include "Component.h"

namespace Shit {
	class GameObject; // 前向声明

	/**
	 * @brief 行为基类
	 * 
	 * 继承自 Component, 拥有三个生命周期
	 */
	class SHIT_API Behavior : public Component {
		friend class GameObject;
	public:
		Behavior() = default;
		~Behavior() override = default;

		// --- 生命周期 ---
		void onAttach() override;
		virtual void onStart();		  // 组件更新时前，执行一次
		virtual void onUpdate();	  // 组件更新时，每帧执行一次
		void onDestroy() override;;

		bool isStarted() const { return m_isStarted; }
		void setStarted(bool isStarted) { m_isStarted = isStarted; }

	protected:
		bool m_isStarted = false; // 是否运行过 onStart 函数
	};
}