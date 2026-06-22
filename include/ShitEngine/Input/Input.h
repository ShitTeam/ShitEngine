#pragma once
#include <unordered_map>
#include <variant>

#include "../Core/Core.h"
#include "../Math.h"
#include "KeyCode.h"

namespace Shit {
	/**
	 * @brief 输入管理类
	 */
	class SHIT_API Input {
	public:
		// --- 成员变量 ---
		bool isKeyDown(const KeyCode code);     // 按键被按下
		bool isKeyPressed(const KeyCode code);  // 按键被持续按下
		bool isKeyReleased(const KeyCode code);       // 按键被释放

		// 鼠标
		bool isMouseButtonDown(const MouseButton code);
		bool isMouseButtonPressed(const MouseButton code);
		bool isMouseButtonReleased(const MouseButton code);
		Vector2 getMousePosition();

		void update(); // 更新

		// --- 处理事件 ---
		void handleEvent(const SDL_Event& event);

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