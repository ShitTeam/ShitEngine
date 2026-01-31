#pragma once
#include "pch.h"
#include <SFML/Graphics.hpp>

namespace Shit {
	/**
	 * @brief 静态时间类
	 */
	class SHIT_API Time {
		friend class Game; // 只允许Game使用Update
	public:
		inline static float GetDeltaTime() { return s_DeltaTime; } // 获取DeltaTime
	private:
		inline static void Update() { // 更新DeltaTime
			s_DeltaTime = s_Clock.restart().asSeconds();
		}

		static sf::Clock s_Clock; // 计时器
		static float s_DeltaTime; 
	};
}