#pragma once

#include "../Core/Core.h"
#include "../Math.h"
#include "KeyCode.h"

#include <array>
#include <string>
#include <unordered_map>
#include <vector>

union SDL_Event;

namespace Shit {
	/**
	 * @brief 输入管理（单例）
	 *
	 * 同时提供两层 API（类似 Unity / Godot）：
	 *
	 * 1. 底层键鼠
	 *    Input::IsKeyDown(KeyCode::Space)
	 *    Input::IsMouseButtonPressed(MouseButton::Left)
	 *
	 * 2. 动作 / 轴映射（配置在 settings.json 的 inputMappings）
	 *    Input::IsActionDown("Jump")
	 *    Input::IsActionPressed("Jump")
	 *    Input::IsActionReleased("Jump")
	 *    Input::GetAxis("Horizontal")   // -1 / 0 / 1
	 *
	 * 命名约定（与 Unity 不同，请注意）：
	 *   Down     = 刚按下（仅一帧）
	 *   Pressed  = 持续按住
	 *   Released = 刚松开（仅一帧）
	 *
	 * 键名使用 SDL 官方 scancode 名（"Space"/"A"/"Left Shift"），
	 * 也接受无空格别名（"LeftShift" → "Left Shift"）。
	 * 鼠标：MouseButton.Left / Right / Middle / XButton1 / XButton2
	 */
	class SHIT_API Input {
	public:
		// --- 底层：键盘 ---
		bool isKeyDown(KeyCode code) const;
		bool isKeyPressed(KeyCode code) const;
		bool isKeyReleased(KeyCode code) const;

		// --- 底层：鼠标 ---
		bool isMouseButtonDown(MouseButton code) const;
		bool isMouseButtonPressed(MouseButton code) const;
		bool isMouseButtonReleased(MouseButton code) const;
		Vector2 getMousePosition() const;
		Vector2 getMouseScroll() const;

		// --- 动作映射（settings.json → inputMappings.actions）---
		bool isActionDown(const std::string& action) const;
		bool isActionPressed(const std::string& action) const;
		bool isActionReleased(const std::string& action) const;

		// --- 轴映射（settings.json → inputMappings.axes）---
		float getAxis(const std::string& axis) const;

		// --- 生命周期 ---
		void initMappings(); ///< 从 Config 预编译动作/轴绑定（Config::Init 之后调用）
		void update();       ///< 帧末：current → previous
		void handleEvent(const SDL_Event& event);

		// --- 静态门面 ---
		static Input& GetInstance();

		static bool IsKeyDown(KeyCode code) { return GetInstance().isKeyDown(code); }
		static bool IsKeyPressed(KeyCode code) { return GetInstance().isKeyPressed(code); }
		static bool IsKeyReleased(KeyCode code) { return GetInstance().isKeyReleased(code); }

		static bool IsMouseButtonDown(MouseButton code) { return GetInstance().isMouseButtonDown(code); }
		static bool IsMouseButtonPressed(MouseButton code) { return GetInstance().isMouseButtonPressed(code); }
		static bool IsMouseButtonReleased(MouseButton code) { return GetInstance().isMouseButtonReleased(code); }
		static Vector2 GetMousePosition() { return GetInstance().getMousePosition(); }
		static Vector2 GetMouseScroll() { return GetInstance().getMouseScroll(); }

		static bool IsActionDown(const std::string& action) { return GetInstance().isActionDown(action); }
		static bool IsActionPressed(const std::string& action) { return GetInstance().isActionPressed(action); }
		static bool IsActionReleased(const std::string& action) { return GetInstance().isActionReleased(action); }
		static float GetAxis(const std::string& axis) { return GetInstance().getAxis(axis); }

		static void InitMappings() { GetInstance().initMappings(); }
		static void Update() { GetInstance().update(); }
		static void HandleEvent(const SDL_Event& event) { GetInstance().handleEvent(event); }

	private:
		enum class BindingType { Key, Mouse };

		struct CompiledBinding {
			BindingType type = BindingType::Key;
			KeyCode key = KeyCode::Unknown;
			MouseButton mouse = MouseButton::Count;
		};

		struct CompiledAction {
			std::vector<CompiledBinding> bindings;
		};

		struct CompiledAxis {
			std::vector<CompiledBinding> negative;
			std::vector<CompiledBinding> positive;
		};

		Input();
		~Input();

		Input(const Input&) = delete;
		Input& operator=(const Input&) = delete;
		Input(Input&&) = delete;
		Input& operator=(Input&&) = delete;

		// 解析配置键名 → 编译后绑定；失败返回 false 并打 WARN
		bool compileBinding(const std::string& name, CompiledBinding& out) const;
		static KeyCode parseKeyCode(const std::string& name);
		static MouseButton parseMouseButton(const std::string& name);
		static std::string normalizeKeyName(const std::string& name);

		bool isBindingDown(const CompiledBinding& b) const;
		bool isBindingPressed(const CompiledBinding& b) const;
		bool isBindingReleased(const CompiledBinding& b) const;

		bool anyBindingDown(const std::vector<CompiledBinding>& bindings) const;
		bool anyBindingPressed(const std::vector<CompiledBinding>& bindings) const;
		bool anyBindingReleased(const std::vector<CompiledBinding>& bindings) const;

		// 底层状态
		std::array<bool, static_cast<int>(KeyCode::Count)> m_currentKeys{};
		std::array<bool, static_cast<int>(KeyCode::Count)> m_previousKeys{};
		std::array<bool, static_cast<int>(MouseButton::Count)> m_currentMouseButtons{};
		std::array<bool, static_cast<int>(MouseButton::Count)> m_previousMouseButtons{};
		Vector2 m_mouseScroll{ 0.0f, 0.0f };

		// 预编译映射
		std::unordered_map<std::string, CompiledAction> m_actions;
		std::unordered_map<std::string, CompiledAxis> m_axes;
		bool m_mappingsReady = false;
	};
}
