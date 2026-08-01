#include "ShitEngine/Core/pch.h"
#include "ShitEngine/Core/EngineContext.h"
#include "ShitEngine/Core/TextInputGate.h"

#include "ShitEngine/UI/UITextInput.h"
#include "ShitEngine/Input/KeyCode.h"
#include "ShitEngine/Core/Window.h"
#include "ShitEngine/Core/Log.h"

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_keyboard.h>

namespace Shit {
	TextInputGate& TextInputGate::GetInstance() {
		return EngineContext::current().textInputGate;
	}

	bool TextInputGate::HasFocus() {
		return GetInstance().m_focused != nullptr;
	}

	void TextInputGate::requestFocus(UITextInput* control) {
		if (!control) return;
		if (m_focused == control) return;

		if (m_focused) {
			m_focused->setFocused(false);
		}

		// 新聚焦会话开始：清空已消费按键记录（上一会话遗留的 UP 已随焦点释放消费完毕）
		m_focusConsumedKeys.clear();

		SDL_Window* window = Window::GetWindow();
		if (window) {
			SDL_ClearComposition(window);
			SDL_StartTextInput(window);
		}
		m_focused = control;
		control->setFocused(true);
	}

	void TextInputGate::releaseFocus(UITextInput* control) {
		if (!control || m_focused != control) return;

		SDL_Window* window = Window::GetWindow();
		if (window) {
			SDL_ClearComposition(window);
			SDL_StopTextInput(window);
		}
		m_focused = nullptr;
		control->setFocused(false);
	}

	void TextInputGate::clearFocus() {
		releaseFocus(m_focused);
	}

	void TextInputGate::updateCursorRect(const SDL_Rect& rect, int cursor) {
		SDL_Window* window = Window::GetWindow();
		if (window) {
			SDL_SetTextInputArea(window, &rect, cursor);
		}
	}

	bool TextInputGate::handleEvent(const SDL_Event& event) {
		// 聚焦期间消费过的按键抬起：即使焦点已在 KEY_DOWN 时被释放（如 Escape），其 KEY_UP
		// 仍须消费，避免只把 UP 转发给全局 Input 造成按下/抬起失衡（误触发 IsKeyReleased）。
		if (event.type == SDL_EVENT_KEY_UP) {
			KeyCode upKey = static_cast<KeyCode>(event.key.scancode);
			if (m_focusConsumedKeys.erase(upKey)) return true;
		}

		if (!m_focused) return false;

		switch (event.type) {
			case SDL_EVENT_TEXT_INPUT:
				m_focused->onTextInput(event.text.text);
				return true;
			case SDL_EVENT_TEXT_EDITING:
				m_focused->onTextEditing(event.edit.text, event.edit.start, event.edit.length);
				return true;
			case SDL_EVENT_KEY_DOWN: {
				KeyCode key = static_cast<KeyCode>(event.key.scancode);
				bool shift = (event.key.mod & SDL_KMOD_SHIFT) != 0;
				bool ctrl  = (event.key.mod & SDL_KMOD_CTRL) != 0;
				// 导航键/编辑键（含 Return：多行 UITextArea 通过 onKeyDown 的 Return 分支插入换行；
				// TEXT_INPUT 的 '\n' 已在 UITextInput::onTextInput 中被控字符门拦截，不会双插入）
				bool navKeys = (key == KeyCode::Left || key == KeyCode::Right ||
					key == KeyCode::Home || key == KeyCode::End ||
					key == KeyCode::Backspace || key == KeyCode::Delete ||
					key == KeyCode::Up || key == KeyCode::Down ||
					key == KeyCode::Return);
				if (navKeys) {
					m_focused->onKeyDown(key, shift, ctrl);
					m_focusConsumedKeys.insert(key);
					return true;  // 已消费，避免编辑键同时进入全局 Input
				} else if (key == KeyCode::Escape) {
					releaseFocus(m_focused);
					m_focusConsumedKeys.insert(key);  // Escape 的 UP 也需消费
					return true;  // 失焦键已消费，避免当帧触发绑定 Escape 的游戏动作
				}
				// 聚焦期间其余按键一律消费：避免打字时误触发绑定到这些键的游戏动作
				//（如 WASD 移动、空格跳跃）。按键 UP 由上方 KEY_UP 分支配对消费。
				m_focusConsumedKeys.insert(key);
				return true;
			}
			default:
				return false;
		}
	}
}
