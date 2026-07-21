#include "ShitEngine/UI/UITextBox.h"
#include "ShitEngine/Core/Log.h"

#include <SDL3/SDL_render.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <algorithm>
#include <cmath>

namespace Shit {
	namespace {
		int measureTextWidth(TTF_Font* font, const char* text, size_t length) {
			if (!font || !text || length == 0) return 0;
			int w = 0, h = 0;
			TTF_GetStringSize(font, text, length, &w, &h);
			return w;
		}

		void renderTextTo(SDL_Renderer* renderer, TTF_Font* font,
			const std::string& text, SDL_Color color, float x, float y)
		{
			if (text.empty() || !font) return;
			SDL_Surface* surface = TTF_RenderText_Blended(font, text.c_str(), text.length(), color);
			if (!surface) return;
			SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surface);
			if (tex) {
				SDL_FRect dst{ x, y, static_cast<float>(surface->w), static_cast<float>(surface->h) };
				SDL_RenderTexture(renderer, tex, nullptr, &dst);
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
			// 统计新文本中将要插入的 UTF-8 字符数
			size_t newChars = SDL_utf8strlen(utf8.c_str());
			// 选区被替换时应减去已选区的字符数
			if (m_cursor != m_selectionAnchor) {
				size_t selBytes = std::max(m_cursor, m_selectionAnchor) - std::min(m_cursor, m_selectionAnchor);
				// 估算选区字符数（近似：字节数 ≥ 字符数，但对于 ASCII 相等，中文字符数 < 字节数）
				std::string selStr = m_text.substr(std::min(m_cursor, m_selectionAnchor), selBytes);
				currentChars -= SDL_utf8strlen(selStr.c_str());
			}
			if (currentChars + newChars > m_characterLimit) {
				return; // 超过限制，拒绝插入
			}
		}
		UITextInput::insertText(utf8);
	}

	void UITextBox::onRender(SDL_Renderer* renderer, const SDL_FRect& screenRect) {
		TTF_Font* font = acquireFont();
		if (!font) return;

		float textY = screenRect.y + (screenRect.h - m_fontHeight) * 0.5f;

		const std::string& displayText = (!m_text.empty() || m_isFocused) ? m_text : m_placeholder;
		SDL_Color textColor = (!m_text.empty() || m_isFocused)
			? SDL_Color{ m_textColor.red, m_textColor.green, m_textColor.blue, m_textColor.alpha }
			: SDL_Color{ m_placeholderColor.red, m_placeholderColor.green, m_placeholderColor.blue, m_placeholderColor.alpha };

		SDL_Rect clipRect{
			static_cast<int>(screenRect.x),
			static_cast<int>(screenRect.y),
			static_cast<int>(screenRect.w),
			static_cast<int>(screenRect.h)
		};

		// 水平滚动偏移
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

		// 选区高亮
		if (m_isFocused && m_cursor != m_selectionAnchor && !displayText.empty()) {
			size_t selStart = std::min(m_cursor, m_selectionAnchor);
			size_t selEnd = std::max(m_cursor, m_selectionAnchor);
			std::string beforeSel = displayText.substr(0, selStart);
			std::string selText = displayText.substr(selStart, selEnd - selStart);

			int beforeW = measureTextWidth(font, beforeSel.c_str(), beforeSel.length());
			int selW = measureTextWidth(font, selText.c_str(), selText.length());

			SDL_SetRenderDrawColor(renderer, 80, 140, 220, 120);
			SDL_FRect selRect{
				screenRect.x + padding + static_cast<float>(beforeW) + scrollX,
				textY,
				static_cast<float>(selW),
				m_fontHeight
			};
			SDL_RenderFillRect(renderer, &selRect);
		}

		// 绘制文本
		{
			SDL_SetRenderClipRect(renderer, &clipRect);
			renderTextTo(renderer, font, displayText, textColor,
				screenRect.x + padding + scrollX, textY);
			SDL_SetRenderClipRect(renderer, nullptr);
		}

		// 闪烁光标（含 preedit 文本）
		if (m_isFocused) {
			SDL_SetRenderClipRect(renderer, &clipRect);

			Uint64 ticks = SDL_GetTicks();
			bool cursorVisible = (ticks / 500) % 2 == 0;

			std::string beforeCursor = m_text.substr(0, m_cursor);
			int preX = measureTextWidth(font, beforeCursor.c_str(), beforeCursor.length());
			float cursorPxX = screenRect.x + padding + static_cast<float>(preX) + scrollX;

			if (cursorVisible && !m_preedit.empty()) {
				// 渲染 IME 组合文本（预编辑阶段，灰色表示尚未提交）
				SDL_Color preColor{ 60, 60, 60, 255 };
				renderTextTo(renderer, font, m_preedit, preColor, cursorPxX, textY);

				// 组合文本下方画下划线
				int preW = measureTextWidth(font, m_preedit.c_str(), m_preedit.length());
				SDL_SetRenderDrawColor(renderer, 100, 100, 100, 200);
				SDL_FRect preeditRect{
					cursorPxX,
					textY + m_fontHeight - 2.0f,
					static_cast<float>(preW),
					2.0f
				};
				SDL_RenderFillRect(renderer, &preeditRect);
			} else if (cursorVisible) {
				SDL_SetRenderDrawColor(renderer,
					m_cursorColor.red, m_cursorColor.green, m_cursorColor.blue, m_cursorColor.alpha);
				SDL_FRect cursorRect{
					cursorPxX,
					textY,
					2.0f,
					m_fontHeight
				};
				SDL_RenderFillRect(renderer, &cursorRect);
			}

			SDL_SetRenderClipRect(renderer, nullptr);
		}

		m_isDirty = false;
	}
}
