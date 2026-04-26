#pragma once
#include <unordered_map>
#include <variant>

#include "Core.h"
#include "../Math.h"
#include "Window.h"
#include <SFML/Window.hpp>

namespace Shit {
	using Scancode = sf::Keyboard::Scancode;
	using Mouse = sf::Mouse::Button;

	enum class KeyState { // 按键状态
		None,             // 空
		Down,             // 按键被按下
		Pressed,          // 按键被持续按下
		Released          // 按键被释放
	};

	/**
	 * @brief 输入管理类
	 */
	class SHIT_API Input {
	public:
		// --- 成员变量 ---
		bool isKeyDown(const Scancode code);     // 按键被按下
		bool isKeyPressed(const Scancode code);  // 按键被持续按下
		bool isKeyReleased(const Scancode code);       // 按键被释放
		bool isKeyDown(const Mouse code);
		bool isKeyPressed(const Mouse code);
		bool isKeyReleased(const Mouse code);

		void update(); // 更新

		// --- 拉取事件 ---
		void handleEvent(const sf::Event::KeyPressed& keyPressed);
		void handleEvent(const sf::Event::KeyReleased& keyReleased);
		void handleEvent(const sf::Event::MouseButtonPressed& mouseButtonPressed);
		void handleEvent(const sf::Event::MouseButtonReleased& mouseButtonReleased);

		// --- 静态API ---
		static Input& GetInstance();
		inline static bool IsKeyDown(const auto code) { return GetInstance().isKeyDown(code); }
		inline static bool IsKeyPressed(const auto code) { return GetInstance().isKeyPressed(code); }
		inline static bool IsKeyReleased(const auto code) { return GetInstance().isKeyReleased(code); }
		inline static void HandleEvent(const auto& event) { GetInstance().handleEvent(event); }
		inline static void Update() { GetInstance().update(); }
		//inline static Vector2 GetMousePosition() { return Math::ToGLM(sf::Mouse::getPosition(Window::GetWindow())); }
	private:

		std::unordered_map<std::variant<Scancode, Mouse>, KeyState> m_keyStates; // 按键状态表
		
		Input();
		~Input();

		Input(const Input&) = delete;
		Input& operator=(const Input&) = delete;
		Input(Input&&) = delete;
		Input& operator=(Input&&) = delete;
	};
}