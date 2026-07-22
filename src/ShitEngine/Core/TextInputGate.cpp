#include "ShitEngine/Core/pch.h"
#include "ShitEngine/Core/TextInputGate.h"

#include "ShitEngine/UI/UITextInput.h"
#include "ShitEngine/Core/Window.h"
#include "ShitEngine/Core/Log.h"

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_scancode.h>

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
				if (event.key.repeat) break;
				SDL_Scancode sc = event.key.scancode;
				bool shift = (event.key.mod & SDL_KMOD_SHIFT) != 0;
				bool ctrl  = (event.key.mod & SDL_KMOD_CTRL) != 0;
				// 导航键/编辑键（RETURN 由 TEXT_INPUT 处理，避免双插入）
				bool navKeys = (sc == SDL_SCANCODE_LEFT || sc == SDL_SCANCODE_RIGHT ||
					sc == SDL_SCANCODE_HOME || sc == SDL_SCANCODE_END ||
					sc == SDL_SCANCODE_BACKSPACE || sc == SDL_SCANCODE_DELETE ||
					sc == SDL_SCANCODE_UP || sc == SDL_SCANCODE_DOWN);
				if (navKeys) {
					m_focused->onKeyDown(sc, shift, ctrl);
				} else if (sc == SDL_SCANCODE_ESCAPE) {
					releaseFocus(m_focused);
				}
				break;
			}
			default:
				break;
		}
	}
}
