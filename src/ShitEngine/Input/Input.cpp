#include "ShitEngine/Core/pch.h"
#include "ShitEngine/Input/Input.h"
#include "ShitEngine/Core/Config.h"
#include "ShitEngine/Core/Log.h"
#include "ShitEngine/Render/Renderer.h"

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_mouse.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_scancode.h>

#include <cctype>

namespace Shit {
	Input::Input() = default;
	Input::~Input() = default;

	Input& Input::GetInstance() {
		static Input instance;
		return instance;
	}

	// =========================================================================
	// 底层：键盘 / 鼠标
	// =========================================================================

	bool Input::isKeyDown(KeyCode code) const {
		int scancode = static_cast<int>(code);
		if (scancode < 0 || scancode >= SDL_SCANCODE_COUNT) return false;
		return m_currentKeys[scancode] && !m_previousKeys[scancode];
	}

	bool Input::isKeyPressed(KeyCode code) const {
		int scancode = static_cast<int>(code);
		if (scancode < 0 || scancode >= SDL_SCANCODE_COUNT) return false;
		return m_currentKeys[scancode];
	}

	bool Input::isKeyReleased(KeyCode code) const {
		int scancode = static_cast<int>(code);
		if (scancode < 0 || scancode >= SDL_SCANCODE_COUNT) return false;
		return !m_currentKeys[scancode] && m_previousKeys[scancode];
	}

	bool Input::isMouseButtonDown(MouseButton code) const {
		int buttonIndex = static_cast<int>(code);
		if (buttonIndex < 0 || buttonIndex >= static_cast<int>(MouseButton::Count)) return false;
		return m_currentMouseButtons[buttonIndex] && !m_previousMouseButtons[buttonIndex];
	}

	bool Input::isMouseButtonPressed(MouseButton code) const {
		int buttonIndex = static_cast<int>(code);
		if (buttonIndex < 0 || buttonIndex >= static_cast<int>(MouseButton::Count)) return false;
		return m_currentMouseButtons[buttonIndex];
	}

	bool Input::isMouseButtonReleased(MouseButton code) const {
		int buttonIndex = static_cast<int>(code);
		if (buttonIndex < 0 || buttonIndex >= static_cast<int>(MouseButton::Count)) return false;
		return !m_currentMouseButtons[buttonIndex] && m_previousMouseButtons[buttonIndex];
	}

	Vector2 Input::getMousePosition() const {
		float windowX = 0.0f, windowY = 0.0f;
		SDL_GetMouseState(&windowX, &windowY);

		// 物理窗口坐标 → 逻辑分辨率坐标（letterbox 后必须转换）
		Vector2 position{ windowX, windowY };
		if (auto* renderer = Renderer::GetRenderer()) {
			float logicalX = 0.0f, logicalY = 0.0f;
			if (SDL_RenderCoordinatesFromWindow(renderer, windowX, windowY, &logicalX, &logicalY)) {
				position = { logicalX, logicalY };
			}
		}
		return position;
	}

	Vector2 Input::getMouseScroll() const {
		return m_mouseScroll;
	}

	// =========================================================================
	// 键名解析
	// =========================================================================

	std::string Input::normalizeKeyName(const std::string& name) {
		// CamelCase → 空格分隔：LeftShift → Left Shift（匹配 SDL 官方 scancode 名）
		if (name.empty()) return name;
		std::string out;
		out.reserve(name.size() + 4);
		out.push_back(name[0]);
		for (size_t i = 1; i < name.size(); ++i) {
			unsigned char prev = static_cast<unsigned char>(name[i - 1]);
			unsigned char cur = static_cast<unsigned char>(name[i]);
			if (std::isupper(cur) && std::islower(prev)) {
				out.push_back(' ');
			}
			out.push_back(static_cast<char>(cur));
		}
		return out;
	}

	MouseButton Input::parseMouseButton(const std::string& name) {
		if (name == "MouseButton.Left" || name == "Mouse.Left") return MouseButton::Left;
		if (name == "MouseButton.Right" || name == "Mouse.Right") return MouseButton::Right;
		if (name == "MouseButton.Middle" || name == "Mouse.Middle") return MouseButton::Middle;
		if (name == "MouseButton.XButton1" || name == "Mouse.X1") return MouseButton::XButton1;
		if (name == "MouseButton.XButton2" || name == "Mouse.X2") return MouseButton::XButton2;
		return MouseButton::Count;
	}

	KeyCode Input::parseKeyCode(const std::string& name) {
		// 鼠标绑定不走 KeyCode
		if (parseMouseButton(name) != MouseButton::Count) return KeyCode::Unknown;

		// 常用别名（配置友好）
		if (name == "Enter") {
			return static_cast<KeyCode>(SDL_SCANCODE_RETURN);
		}
		if (name == "LeftControl" || name == "LeftCtrl") {
			return static_cast<KeyCode>(SDL_SCANCODE_LCTRL);
		}
		if (name == "RightControl" || name == "RightCtrl") {
			return static_cast<KeyCode>(SDL_SCANCODE_RCTRL);
		}
		if (name == "LeftAlt")  return static_cast<KeyCode>(SDL_SCANCODE_LALT);
		if (name == "RightAlt") return static_cast<KeyCode>(SDL_SCANCODE_RALT);
		if (name == "LeftGui" || name == "LeftSuper" || name == "LeftMeta") {
			return static_cast<KeyCode>(SDL_SCANCODE_LGUI);
		}
		if (name == "RightGui" || name == "RightSuper" || name == "RightMeta") {
			return static_cast<KeyCode>(SDL_SCANCODE_RGUI);
		}

		// 1) 原名
		SDL_Scancode sc = SDL_GetScancodeFromName(name.c_str());
		if (sc != SDL_SCANCODE_UNKNOWN) return static_cast<KeyCode>(sc);

		// 2) CamelCase → "Left Shift"
		std::string spaced = normalizeKeyName(name);
		if (spaced != name) {
			sc = SDL_GetScancodeFromName(spaced.c_str());
			if (sc != SDL_SCANCODE_UNKNOWN) return static_cast<KeyCode>(sc);
		}

		return KeyCode::Unknown;
	}

	bool Input::compileBinding(const std::string& name, CompiledBinding& out) const {
		MouseButton mb = parseMouseButton(name);
		if (mb != MouseButton::Count) {
			out.type = BindingType::Mouse;
			out.mouse = mb;
			out.key = KeyCode::Unknown;
			return true;
		}

		KeyCode kc = parseKeyCode(name);
		if (kc != KeyCode::Unknown) {
			out.type = BindingType::Key;
			out.key = kc;
			out.mouse = MouseButton::Count;
			return true;
		}

		ST_CORE_WARN("Input: 无法识别绑定键名 \"{}\"，已忽略", name);
		return false;
	}

	// =========================================================================
	// 映射编译
	// =========================================================================

	void Input::initMappings() {
		m_actions.clear();
		m_axes.clear();

		const auto& cfg = Config::GetInputMappingsConfig();

		for (const auto& [actionName, binding] : cfg.actions) {
			CompiledAction compiled;
			for (const auto& keyName : binding.keys) {
				CompiledBinding b;
				if (compileBinding(keyName, b)) {
					compiled.bindings.push_back(b);
				}
			}
			if (compiled.bindings.empty()) {
				ST_CORE_WARN("Input: 动作 \"{}\" 没有任何有效绑定", actionName);
			}
			m_actions[actionName] = std::move(compiled);
		}

		for (const auto& [axisName, binding] : cfg.axes) {
			CompiledAxis compiled;
			for (const auto& keyName : binding.negative) {
				CompiledBinding b;
				if (compileBinding(keyName, b)) {
					compiled.negative.push_back(b);
				}
			}
			for (const auto& keyName : binding.positive) {
				CompiledBinding b;
				if (compileBinding(keyName, b)) {
					compiled.positive.push_back(b);
				}
			}
			if (compiled.negative.empty() && compiled.positive.empty()) {
				ST_CORE_WARN("Input: 轴 \"{}\" 没有任何有效绑定", axisName);
			}
			m_axes[axisName] = std::move(compiled);
		}

		m_mappingsReady = true;
		ST_CORE_INFO("Input: 已加载 {} 个动作、{} 个轴", m_actions.size(), m_axes.size());
	}

	// =========================================================================
	// 编译后绑定查询
	// =========================================================================

	bool Input::isBindingDown(const CompiledBinding& b) const {
		if (b.type == BindingType::Key) return isKeyDown(b.key);
		return isMouseButtonDown(b.mouse);
	}

	bool Input::isBindingPressed(const CompiledBinding& b) const {
		if (b.type == BindingType::Key) return isKeyPressed(b.key);
		return isMouseButtonPressed(b.mouse);
	}

	bool Input::isBindingReleased(const CompiledBinding& b) const {
		if (b.type == BindingType::Key) return isKeyReleased(b.key);
		return isMouseButtonReleased(b.mouse);
	}

	bool Input::anyBindingDown(const std::vector<CompiledBinding>& bindings) const {
		for (const auto& b : bindings) {
			if (isBindingDown(b)) return true;
		}
		return false;
	}

	bool Input::anyBindingPressed(const std::vector<CompiledBinding>& bindings) const {
		for (const auto& b : bindings) {
			if (isBindingPressed(b)) return true;
		}
		return false;
	}

	bool Input::anyBindingReleased(const std::vector<CompiledBinding>& bindings) const {
		for (const auto& b : bindings) {
			if (isBindingReleased(b)) return true;
		}
		return false;
	}

	// =========================================================================
	// 动作 / 轴 API
	// =========================================================================

	bool Input::isActionDown(const std::string& action) const {
		if (!m_mappingsReady) return false;
		auto it = m_actions.find(action);
		if (it == m_actions.end()) return false;
		return anyBindingDown(it->second.bindings);
	}

	bool Input::isActionPressed(const std::string& action) const {
		if (!m_mappingsReady) return false;
		auto it = m_actions.find(action);
		if (it == m_actions.end()) return false;
		return anyBindingPressed(it->second.bindings);
	}

	bool Input::isActionReleased(const std::string& action) const {
		if (!m_mappingsReady) return false;
		auto it = m_actions.find(action);
		if (it == m_actions.end()) return false;
		// 直接复用底层 Released，不依赖额外 prev 快照
		return anyBindingReleased(it->second.bindings);
	}

	float Input::getAxis(const std::string& axis) const {
		if (!m_mappingsReady) return 0.0f;
		auto it = m_axes.find(axis);
		if (it == m_axes.end()) return 0.0f;

		float val = 0.0f;
		if (anyBindingPressed(it->second.negative)) val -= 1.0f;
		if (anyBindingPressed(it->second.positive)) val += 1.0f;
		return val;
	}

	// =========================================================================
	// 事件 / 帧更新
	// =========================================================================

	void Input::update() {
		m_previousKeys = m_currentKeys;
		m_previousMouseButtons = m_currentMouseButtons;
		m_mouseScroll = { 0.0f, 0.0f };
	}

	void Input::handleEvent(const SDL_Event& event) {
		switch (event.type) {
			case SDL_EVENT_KEY_DOWN:
				if (event.key.scancode < SDL_SCANCODE_COUNT && !event.key.repeat) {
					m_currentKeys[event.key.scancode] = true;
				}
				break;
			case SDL_EVENT_KEY_UP:
				if (event.key.scancode < SDL_SCANCODE_COUNT) {
					m_currentKeys[event.key.scancode] = false;
				}
				break;
			case SDL_EVENT_MOUSE_BUTTON_DOWN:
				if (event.button.button < static_cast<int>(MouseButton::Count)) {
					m_currentMouseButtons[event.button.button] = true;
				}
				break;
			case SDL_EVENT_MOUSE_BUTTON_UP:
				if (event.button.button < static_cast<int>(MouseButton::Count)) {
					m_currentMouseButtons[event.button.button] = false;
				}
				break;
			case SDL_EVENT_MOUSE_WHEEL:
				m_mouseScroll.x = event.wheel.x;
				m_mouseScroll.y = event.wheel.y;
				break;
			default:
				break;
		}
	}
}
