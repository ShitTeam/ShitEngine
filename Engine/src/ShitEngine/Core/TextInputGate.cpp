#include "ShitEngine/Core/pch.h"
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
		static TextInputGate instance;
		return instance;
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

	void TextInputGate::handleEvent(const SDL_Event& event) {
		if (!m_focused) return;

		switch (event.type) {
			case SDL_EVENT_TEXT_INPUT:
				m_focused->onTextInput(event.text.text);
				break;
			case SDL_EVENT_TEXT_EDITING:
				m_focused->onTextEditing(event.edit.text, event.edit.start, event.edit.length);
				break;
			case SDL_EVENT_KEY_DOWN: {
				KeyCode key = static_cast<KeyCode>(event.key.scancode);
				bool shift = (event.key.mod & SDL_KMOD_SHIFT) != 0;
				bool ctrl  = (event.key.mod & SDL_KMOD_CTRL) != 0;
				// 导航键/编辑键（RETURN 由 TEXT_INPUT 处理，避免双插入）
				bool navKeys = (key == KeyCode::Left || key == KeyCode::Right ||
					key == KeyCode::Home || key == KeyCode::End ||
					key == KeyCode::Backspace || key == KeyCode::Delete ||
					key == KeyCode::Up || key == KeyCode::Down);
				// 导航键允许 repeat（长按退格/方向键连续操作），非导航键跳过
				if (!navKeys && event.key.repeat) break;
				if (navKeys) {
					m_focused->onKeyDown(key, shift, ctrl);
				} else if (key == KeyCode::Escape) {
					releaseFocus(m_focused);
				}
				break;
			}
			default:
				break;
		}
	}
}
