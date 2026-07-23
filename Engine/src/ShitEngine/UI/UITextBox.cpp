#include "ShitEngine/Core/pch.h"
#include "ShitEngine/UI/UITextBox.h"
#include "ShitEngine/Render/Renderer.h"
#include "ShitEngine/Core/Log.h"
#include "ShitEngine/Core/TextInputGate.h"

#include <SDL3_ttf/SDL_ttf.h>

namespace Shit {
	namespace {
		int measureTextWidth(TTF_Font* font, const char* text, size_t length) {
			if (!font || !text || length == 0) return 0;
			int w = 0, h = 0;
			TTF_GetStringSize(font, text, length, &w, &h);
			return w;
		}

		void renderTextTo(TTF_Font* font, const std::string& text, SDL_Color color, float x, float y) {
			if (text.empty() || !font) return;
			SDL_Surface* surface = TTF_RenderText_Blended(font, text.c_str(), text.length(), color);
			if (!surface) return;
			SDL_Texture* tex = Renderer::CreateTextureFromSurface(surface);
			if (tex) {
				SDL_FRect dst{ x, y, static_cast<float>(surface->w), static_cast<float>(surface->h) };
				Renderer::DrawTexture(tex, nullptr, dst);
				SDL_DestroyTexture(tex);
			}
			SDL_DestroySurface(surface);
		}
	}

	UITextBox::UITextBox() {
		m_isMultiline = false;
	}

	void UITextBox::insertNewline() {
		// 单行：拒绝换行
	}

	void UITextBox::insertText(const std::string& utf8) {
		// 字符数限制检查：m_characterLimit 为 0 表示不限
		if (m_characterLimit > 0) {
			size_t currentChars = getCharacterCount();
			size_t newChars = SDL_utf8strlen(utf8.c_str());
			if (m_cursor != m_selectionAnchor) {
				size_t selBytes = std::max(m_cursor, m_selectionAnchor) - std::min(m_cursor, m_selectionAnchor);
				std::string selStr = m_text.substr(std::min(m_cursor, m_selectionAnchor), selBytes);
				currentChars -= SDL_utf8strlen(selStr.c_str());
			}
			if (currentChars + newChars > m_characterLimit) {
				return; // 超过限制，拒绝插入
			}
		}
		UITextInput::insertText(utf8);
	}

	void UITextBox::onRender(const SDL_FRect& screenRect) {
		TTF_Font* font = acquireFont();
		if (!font) return;

		float textY = screenRect.y + (screenRect.h - m_fontHeight) * 0.5f;

		const std::string& displayText = (!m_text.empty() || m_isFocused) ? m_text : m_placeholder;
		SDL_Color textColor = (!m_text.empty() || m_isFocused)
			? toSDLColor(m_textColor)
			: toSDLColor(m_placeholderColor);

		SDL_Rect clipRect{
			static_cast<int>(screenRect.x),
			static_cast<int>(screenRect.y),
			static_cast<int>(screenRect.w),
			static_cast<int>(screenRect.h)
		};

		const float padding = 4.0f;
		float scrollX = 0.0f;
		if (m_isFocused && !m_text.empty()) {
			std::string beforeCursor = m_text.substr(0, m_cursor);
			int cursorWidth = measureTextWidth(font, beforeCursor.c_str(), beforeCursor.length());
			float rightEdge = padding + static_cast<float>(cursorWidth);
			float boxRight = screenRect.w - padding;
			if (rightEdge > boxRight) {
				scrollX = boxRight - rightEdge;
			}
			int fullWidth = measureTextWidth(font, m_text.c_str(), m_text.length());
			float maxScroll = std::max(0.0f, static_cast<float>(fullWidth) + padding * 2 - screenRect.w);
			scrollX = std::clamp(scrollX, -maxScroll, 0.0f);
		}

		// 选区高亮（在 SetClipRect 之后绘制，防止溢出）
		if (m_isFocused && m_cursor != m_selectionAnchor && !displayText.empty()) {
			Renderer::SetClipRect(&clipRect);
			size_t selStart = std::min(m_cursor, m_selectionAnchor);
			size_t selEnd = std::max(m_cursor, m_selectionAnchor);
			std::string beforeSel = displayText.substr(0, selStart);
			std::string selText = displayText.substr(selStart, selEnd - selStart);

			int beforeW = measureTextWidth(font, beforeSel.c_str(), beforeSel.length());
			int selW = measureTextWidth(font, selText.c_str(), selText.length());

			Renderer::SetDrawColor({ 80, 140, 220, 120 });
			Renderer::FillRect(SDL_FRect{
				screenRect.x + padding + static_cast<float>(beforeW) + scrollX,
				textY,
				static_cast<float>(selW),
				m_fontHeight
				});
		}

		// 绘制文本
		Renderer::SetClipRect(&clipRect);
		renderTextTo(font, displayText, textColor, screenRect.x + padding + scrollX, textY);
		Renderer::ClearClipRect();

		// 闪烁光标（含 preedit 文本）
		if (m_isFocused) {
			Uint64 ticks = SDL_GetTicks();
			bool cursorVisible = (ticks / 500) % 2 == 0;

			std::string beforeCursor = m_text.substr(0, m_cursor);
			int preX = measureTextWidth(font, beforeCursor.c_str(), beforeCursor.length());
			float cursorPxX = screenRect.x + padding + static_cast<float>(preX) + scrollX;

			// 通知 IME 光标位置（逻辑坐标 → 窗口物理像素坐标）
			float winX1, winY1, winX2, winY2;
			Renderer::RenderCoordinatesToWindow(cursorPxX, textY, &winX1, &winY1);
			Renderer::RenderCoordinatesToWindow(cursorPxX + 2.0f, textY + m_fontHeight, &winX2, &winY2);
			SDL_Rect imeRect{
				static_cast<int>(winX1), static_cast<int>(winY1),
				static_cast<int>(winX2 - winX1), static_cast<int>(winY2 - winY1)
			};
			TextInputGate::GetInstance().updateCursorRect(imeRect, 0);

			Renderer::SetClipRect(&clipRect);

			if (cursorVisible && !m_preedit.empty()) {
				SDL_Color preColor{ 60, 60, 60, 255 };
				renderTextTo(font, m_preedit, preColor, cursorPxX, textY);

				int preW = measureTextWidth(font, m_preedit.c_str(), m_preedit.length());
				Renderer::SetDrawColor({ 100, 100, 100, 200 });
				Renderer::FillRect(SDL_FRect{
					cursorPxX,
					textY + m_fontHeight - 2.0f,
					static_cast<float>(preW),
					2.0f
					});
			} else if (cursorVisible) {
				Renderer::SetDrawColor(m_cursorColor);
				Renderer::FillRect(SDL_FRect{ cursorPxX, textY, 2.0f, m_fontHeight });
			}

			Renderer::ClearClipRect();
		}

		m_isDirty = false;
	}
}