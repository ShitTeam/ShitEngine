#pragma once
#include <unordered_map>
#include <variant>

#include "../Core/Core.h"
#include "../Math.h"
#include "KeyCode.h"

namespace Shit {
	/**
	 * @brief 输入管理类（单例）
	 *
	 * 提供键盘鼠标的三态检测（Down / Pressed / Released）和鼠标位置查询。
	 * 所有 API 均为静态方法。
	 *
	 * 命名提醒：
	 *   IsKeyDown    = 刚按下（瞬间）
	 *   IsKeyPressed = 持续按住（每帧 true 直至松开）
	 *   IsKeyReleased= 松开（瞬间）
	 */
	class SHIT_API Input {
	public:
		bool isKeyDown(const KeyCode code);         ///< 按键刚按下（仅一帧为 true）
		bool isKeyPressed(const KeyCode code);       ///< 按键被持续按住
		bool isKeyReleased(const KeyCode code);      ///< 按键刚松开

		bool isMouseButtonDown(const MouseButton code);     ///< 鼠标按键刚按下
		bool isMouseButtonPressed(const MouseButton code);   ///< 鼠标按键持续按住
		bool isMouseButtonReleased(const MouseButton code);  ///< 鼠标按键刚松开
		Vector2 getMousePosition();                          ///< 鼠标当前位置

		void update(); ///< 帧末更新，把当前帧状态移入上一帧

		void handleEvent(const SDL_Event& event); ///< 处理 SDL 输入事件

		// --- 静态API ---
		static Input& GetInstance();
		inline static bool IsKeyDown(const KeyCode code) { return GetInstance().isKeyDown(code); }
		inline static bool IsKeyPressed(const KeyCode code) { return GetInstance().isKeyPressed(code); }
		inline static bool IsKeyReleased(const KeyCode code) { return GetInstance().isKeyReleased(code); }
		inline static bool IsMouseButtonDown(const MouseButton code) { return GetInstance().isMouseButtonDown(code); }
		inline static bool IsMouseButtonPressed(const MouseButton code) { return GetInstance().isMouseButtonPressed(code); }
		inline static bool IsMouseButtonReleased(const MouseButton code) { return GetInstance().isMouseButtonReleased(code); }
		inline static void HandleEvent(const SDL_Event& event) { GetInstance().handleEvent(event); }
		inline static void Update() { GetInstance().update(); }
		inline static Vector2 GetMousePosition() { return GetInstance().getMousePosition(); }
	private:
		// 按键状态
		std::array<bool, static_cast<int>(KeyCode::Count)> m_currentKeys;
    	std::array<bool, static_cast<int>(KeyCode::Count)> m_previousKeys;

		// 鼠标按键状态
		std::array<bool, static_cast<int>(MouseButton::Count)> m_currentMouseButtons;
		std::array<bool, static_cast<int>(MouseButton::Count)> m_previousMouseButtons;

		// 鼠标滚动增量
		Vector2 m_mouseScroll;
		
		Input();
		~Input();

		Input(const Input&) = delete;
		Input& operator=(const Input&) = delete;
		Input(Input&&) = delete;
		Input& operator=(Input&&) = delete;
	};
}