#pragma once
#include "Component.h"

namespace Shit {
	class GameObject; // 前向声明

	/**
	 * @brief 行为基类
	 * 
	 * 继承自 Component, 拥有三个生命周期
	 */
	class SHIT_API Behavior : Component {
		friend class GameObject;
	public:
		Behavior() = default;
		virtual ~Behavior() = default;

	protected:
		// --- 生命周期 ---
		
		virtual void onStart();		  // 组件启动时，只执行一次
		virtual void onUpdate();	  // 组件更新时，每帧执行一次
		virtual void onDestroy();	  // 组件销毁时，游戏结束前执行一次
	};
}