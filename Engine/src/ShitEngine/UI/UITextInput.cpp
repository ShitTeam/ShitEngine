#include "ShitEngine/Core/pch.h"
#include "ShitEngine/UI/UITextInput.h"
#include "ShitEngine/Core/TextInputGate.h"
#include "ShitEngine/Resource/ResourceManager.h"
#include "ShitEngine/Core/Log.h"

#include <SDL3/SDL_stdinc.h>
#include <SDL3_ttf/SDL_ttf.h>

namespace Shit {
	// ==========================================================================
	// UTF-8 辅助（static methods）
	// ==========================================================================

	size_t UITextInput::byteToChar(const std::string& s, size_t byteOff) {
		size_t chars = 0;
		for (size_t i = 0; i < s.size() && i < byteOff; ) {
			unsigned char c = static_cast<unsigned char>(s[i]);
			i += (c < 0x80) ? 1 : (c < 0xE0) ? 2 : (c < 0xF0) ? 3 : 4;
			++chars;
		}
		return chars;
	}

	size_t UITextInput::charToByte(const std::string& s, size_t charIdx) {
		size_t chars = 0;
		for (size_t i = 0; i < s.size(); ) {
			if (chars >= charIdx) return i;
			unsigned char c = static_cast<unsigned char>(s[i]);
			i += (c < 0x80) ? 1 : (c < 0xE0) ? 2 : (c < 0xF0) ? 3 : 4;
			++chars;
		}
		return s.size();
	}

	namespace {
		size_t moveByteBack(const std::string& s, size_t byteOff) {
			if (byteOff == 0) return 0;
			size_t pos = 0;
			while (pos < s.size()) {
				size_t next = pos;
				unsigned char c = static_cast<unsigned char>(s[next]);
				next += (c < 0x80) ? 1 : (c < 0xE0) ? 2 : (c < 0xF0) ? 3 : 4;
				if (next >= byteOff) return pos;
				pos = next;
			}
			return pos;
		}

		size_t moveByteForward(const std::string& s, size_t byteOff) {
			if (byteOff >= s.size()) return s.size();
			unsigned char c = static_cast<unsigned char>(s[byteOff]);
			size_t inc = (c < 0x80) ? 1 : (c < 0xE0) ? 2 : (c < 0xF0) ? 3 : 4;
			return std::min(byteOff + inc, s.size());
		}
	}

	// ==========================================================================
	// 字体获取（通过 ResourceManager 共享缓存）
	// ==========================================================================

	TTF_Font* UITextInput::acquireFont() {
		if (m_fontPath.empty()) return nullptr;
		TTF_Font* font = ResourceManager::GetFont(m_fontPath, m_fontSize);
		if (font) {
			int h = TTF_GetFontHeight(font);
			m_fontHeight = static_cast<float>(h > 0 ? h : 20);
		}
		return font;
	}

	// ==========================================================================
	// UITextInput
	// ==========================================================================

	UITextInput::~UITextInput() {
		if (m_isFocused) {
			TextInputGate::GetInstance().releaseFocus(this);
		}
	}

	void UITextInput::setText(const std::string& text) {
		m_text = text;
		m_cursor = charToByte(m_text, SDL_utf8strlen(m_text.c_str()));
		m_selectionAnchor = m_cursor;
		m_isDirty = true;
	}

	void UITextInput::setFocused(bool focused) {
		if (m_isFocused == focused) return;
		m_isFocused = focused;
		if (!focused) {
			m_preedit.clear();
			m_preeditStart = 0;
			m_preeditLength = 0;
		}
		m_isDirty = true;
	}

	void UITextInput::onTextInput(const char* utf8Text) {
		if (!utf8Text || utf8Text[0] == '\0') return;
		unsigned char firstByte = static_cast<unsigned char>(utf8Text[0]);
		if (firstByte < 0x20 && utf8Text[0] != '\t') return;

		std::string s(utf8Text);
		if (s.find('\n') != std::string::npos && m_isMultiline) {
			size_t start = 0;
			while (start < s.size()) {
				size_t nl = s.find('\n', start);
				if (nl == std::string::npos) {
					insertText(s.substr(start));
					break;
				} else {
					insertText(s.substr(start, nl - start));
					insertNewline();
					start = nl + 1;
				}
			}
		} else {
			if (s.find('\n') != std::string::npos) {
				s.erase(std::remove(s.begin(), s.end(), '\n'), s.end());
			}
			insertText(s);
		}
		m_preedit.clear();
		m_preeditStart = 0;
		m_preeditLength = 0;
		m_isDirty = true;
	}

	void UITextInput::onTextEditing(const char* utf8Text, int start, int length) {
		m_preedit = utf8Text ? utf8Text : "";
		m_preeditStart = start;
		m_preeditLength = length;
		m_isDirty = true;
	}

	bool UITextInput::onKeyDown(SDL_Scancode scancode, bool shift, bool ctrl) {
		switch (scancode) {
			case SDL_SCANCODE_LEFT:
				moveCursor(-1, shift);
				return true;
			case SDL_SCANCODE_RIGHT:
				moveCursor(1, shift);
				return true;
			case SDL_SCANCODE_HOME:
				moveCursorToBoundary(true, shift);
				return true;
			case SDL_SCANCODE_END:
				moveCursorToBoundary(false, shift);
				return true;
			case SDL_SCANCODE_BACKSPACE:
				if (m_cursor != m_selectionAnchor) {
					deleteSelection();
				} else if (m_cursor > 0) {
					size_t prev = moveByteBack(m_text, m_cursor);
					m_text.erase(prev, m_cursor - prev);
					m_cursor = prev;
					m_selectionAnchor = m_cursor;
					m_isDirty = true;
				}
				return true;
			case SDL_SCANCODE_DELETE:
				if (m_cursor != m_selectionAnchor) {
					deleteSelection();
				} else if (m_cursor < m_text.size()) {
					size_t next = moveByteForward(m_text, m_cursor);
					m_text.erase(m_cursor, next - m_cursor);
					m_selectionAnchor = m_cursor;
					m_isDirty = true;
				}
				return true;
			default:
				return false;
		}
	}

	void UITextInput::insertText(const std::string& utf8) {
		if (utf8.empty()) return;
		deleteSelection();
		m_text.insert(m_cursor, utf8);
		m_cursor += utf8.size();
		m_selectionAnchor = m_cursor;
		m_isDirty = true;
	}

	void UITextInput::insertNewline() {
		// 基类：拒绝换行；UITextArea 覆写
	}

	void UITextInput::deleteSelection() {
		if (m_cursor == m_selectionAnchor) return;
		size_t start = std::min(m_cursor, m_selectionAnchor);
		size_t end = std::max(m_cursor, m_selectionAnchor);
		m_text.erase(start, end - start);
		m_cursor = start;
		m_selectionAnchor = start;
		m_isDirty = true;
	}

	void UITextInput::moveCursor(int deltaChars, bool selecting) {
		if (deltaChars == 0) return;
		if (deltaChars > 0) {
			for (int i = 0; i < deltaChars; ++i)
				m_cursor = moveByteForward(m_text, m_cursor);
		} else {
			for (int i = 0; i < -deltaChars; ++i)
				m_cursor = moveByteBack(m_text, m_cursor);
		}
		if (!selecting) m_selectionAnchor = m_cursor;
		m_isDirty = true;
	}

	void UITextInput::moveCursorToBoundary(bool toStart, bool selecting) {
		m_cursor = toStart ? 0 : m_text.size();
		if (!selecting) m_selectionAnchor = m_cursor;
		m_isDirty = true;
	}

	void UITextInput::clampCursor() {
		if (m_cursor > m_text.size()) {
			m_cursor = m_text.size();
			m_selectionAnchor = m_cursor;
		}
	}

	size_t UITextInput::getCharacterCount() const {
		return SDL_utf8strlen(m_text.c_str());
	}

	void UITextInput::onDetach() {
		if (m_isFocused) {
			TextInputGate::GetInstance().releaseFocus(this);
		}
		UIRendererComponent::onDetach();
	}

	void UITextInput::onDestroy() {
		UIRendererComponent::onDestroy();
	}
}
