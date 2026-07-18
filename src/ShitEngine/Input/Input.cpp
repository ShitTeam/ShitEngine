#include "ShitEngine/Input/Input.h"
#include "ShitEngine/Core/Log.h"
#include "ShitEngine/Core/pch.h"
#include "ShitEngine/Render/Renderer.h"
#include <SDL3/SDL_render.h>

namespace Shit {
	Input::Input() = default;

	Input::~Input() = default;

	Input& Input::GetInstance() { // 获取单例
		static Input instance;
		return instance;
	}

	bool Input::isKeyDown(const KeyCode code) { // 按键被按下
		int scancode = static_cast<int>(code);
		if (scancode < 0 || scancode >= SDL_SCANCODE_COUNT) return false;
		return m_currentKeys[scancode] && !m_previousKeys[scancode];
	}

	bool Input::isKeyPressed(const KeyCode code) { // 按键被持续按下
		int scancode = static_cast<int>(code);
		if (scancode < 0 || scancode >= SDL_SCANCODE_COUNT) return false;
		return m_currentKeys[scancode];
	}

	bool Input::isKeyReleased(const KeyCode code) { // 按键被释放
		int scancode = static_cast<int>(code);
		if (scancode < 0 || scancode >= SDL_SCANCODE_COUNT) return false;
		return !m_currentKeys[scancode] && m_previousKeys[scancode];
	}

	bool Input::isMouseButtonDown(const MouseButton code)
	{
		int buttonIndex = static_cast<int>(code);
		if (buttonIndex < 0 || buttonIndex >= static_cast<int>(MouseButton::Count)) return false;
		return m_currentMouseButtons[buttonIndex] && !m_previousMouseButtons[buttonIndex];
	}

	bool Input::isMouseButtonPressed(const MouseButton code)
	{
		int buttonIndex = static_cast<int>(code);
		if (buttonIndex < 0 || buttonIndex >= static_cast<int>(MouseButton::Count)) return false;
		return m_currentMouseButtons[buttonIndex];
	}

	bool Input::isMouseButtonReleased(const MouseButton code)
	{
		int buttonIndex = static_cast<int>(code);
		if (buttonIndex < 0 || buttonIndex >= static_cast<int>(MouseButton::Count)) return false;
		return !m_currentMouseButtons[buttonIndex] && m_previousMouseButtons[buttonIndex];
	}

	Vector2 Input::getMousePosition()
	{
		float windowX, windowY;
		SDL_GetMouseState(&windowX, &windowY);

		// SDL_GetMouseState 返回窗口物理像素坐标，但渲染走 SDL_SetRenderLogicalPresentation
		// 的逻辑坐标系（默认 1280×720，letterbox 后还可能有黑边）。UI Raycasting 与游戏世界
		// 命中检测都基于逻辑坐标，因此必须转换，否则窗口缩放/非整数倍时点击会偏移。
		Vector2 position{ windowX, windowY };
		if (auto* renderer = Renderer::GetRenderer()) {
			float logicalX = 0.0f, logicalY = 0.0f;
			if (SDL_RenderCoordinatesFromWindow(renderer, windowX, windowY, &logicalX, &logicalY)) {
				position = { logicalX, logicalY };
			}
		}
		return position;
	}

	void Input::update() { // 更新
		m_previousKeys = m_currentKeys;
		m_previousMouseButtons = m_currentMouseButtons;

		m_mouseScroll = { 0.0f, 0.0f };
	}

	void Input::handleEvent(const SDL_Event& event) {
		switch (event.type) {
			// --- 键盘按键事件 ---
			case SDL_EVENT_KEY_DOWN:
				if (event.key.scancode < SDL_SCANCODE_COUNT) {
					// 如果是按键重复触发（长按），忽略它，由我们的轮询逻辑自己处理
					if (!event.key.repeat) {
						m_currentKeys[event.key.scancode] = true;
					}
				}
				break;

			case SDL_EVENT_KEY_UP:
				if (event.key.scancode < SDL_SCANCODE_COUNT) {
					m_currentKeys[event.key.scancode] = false;
				}
				break;

			// --- 鼠标按键事件 ---
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

			// --- 鼠标滚轮事件 ---
			case SDL_EVENT_MOUSE_WHEEL:
				m_mouseScroll.x = event.wheel.x;
				m_mouseScroll.y = event.wheel.y;
				break;
		}
	}
}
