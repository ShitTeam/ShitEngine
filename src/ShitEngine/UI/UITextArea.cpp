#include "ShitEngine/Core/pch.h"
#include "ShitEngine/UI/UITextArea.h"
#include "ShitEngine/Render/Renderer.h"
#include "ShitEngine/Core/Log.h"
#include "ShitEngine/Core/TextInputGate.h"

#include <SDL3_ttf/SDL_ttf.h>
#include <cmath>

namespace Shit {
	namespace {
		int measureWidth(TTF_Font* font, const char* text, size_t length) {
			if (!font || !text || length == 0) return 0;
			int w = 0, h = 0;
			TTF_GetStringSize(font, text, length, &w, &h);
			return w;
		}

		std::vector<std::string> splitLines(const std::string& text) {
			std::vector<std::string> lines;
			std::istringstream stream(text);
			std::string line;
			while (std::getline(stream, line)) {
				if (!line.empty() && line.back() == '\r')
					line.pop_back();
				lines.push_back(line);
			}
			if (text.empty() || text.back() == '\n')
				lines.push_back("");
			return lines;
		}

		void locateCursor(const std::string& text, size_t cursorByte,
			const std::vector<std::string>& lines, int& lineIdx, size_t& lineOffset)
		{
			size_t pos = 0;
			for (int i = 0; i < static_cast<int>(lines.size()) && pos < text.size(); ++i) {
				const std::string& line = lines[i];
				if (pos == cursorByte) {
					lineIdx = i; lineOffset = 0; return;
				}
				size_t lineByte = line.size();
				size_t lineEnd = pos + lineByte;
				if (cursorByte <= lineEnd) {
					lineIdx = i; lineOffset = cursorByte - pos; return;
				}
				pos = lineEnd + 1;
			}
			lineIdx = static_cast<int>(lines.size()) - 1;
			lineOffset = lines.empty() ? 0 : lines.back().size();
		}

		void renderLineTo(TTF_Font* font, const std::string& text, SDL_Color color, float x, float y) {
			if (text.empty() || !font) return;
			SDL_Surface* surf = TTF_RenderText_Blended(font, text.c_str(), text.length(), color);
			if (!surf) return;
			SDL_Texture* tex = Renderer::CreateTextureFromSurface(surf);
			if (tex) {
				SDL_FRect dst{ x, y, static_cast<float>(surf->w), static_cast<float>(surf->h) };
				Renderer::DrawTexture(tex, nullptr, dst);
				SDL_DestroyTexture(tex);
			}
			SDL_DestroySurface(surf);
		}
	}

	UITextArea::UITextArea() {
		m_isMultiline = true;
	}

	// ── UP / DOWN 跨行导航（用当前行 + lineOff 计算列位置，避免全文偏移） ──
	bool UITextArea::onKeyDown(SDL_Scancode scancode, bool shift, bool ctrl) {
		switch (scancode) {
			case SDL_SCANCODE_UP: {
				if (!shift) m_selectionAnchor = m_cursor;
				auto lines = splitLines(m_text);
				int lineIdx = 0; size_t lineOff = 0;
				locateCursor(m_text, m_cursor, lines, lineIdx, lineOff);
				if (lineIdx > 0) {
					size_t curChar = UITextInput::byteToChar(lines[lineIdx], lineOff);
					--lineIdx;
					size_t pos = 0;
					for (int i = 0; i < lineIdx; ++i)
						pos += lines[i].size() + 1;
					const std::string& prevLine = lines[lineIdx];
					size_t newOff = UITextInput::charToByte(prevLine,
						std::min(curChar, UITextInput::byteToChar(prevLine, prevLine.size())));
					m_cursor = pos + newOff;
					if (!shift) m_selectionAnchor = m_cursor;
					m_isDirty = true;
				}
				return true;
			}
			case SDL_SCANCODE_DOWN: {
				if (!shift) m_selectionAnchor = m_cursor;
				auto lines = splitLines(m_text);
				int lineIdx = 0; size_t lineOff = 0;
				locateCursor(m_text, m_cursor, lines, lineIdx, lineOff);
				if (lineIdx < static_cast<int>(lines.size()) - 1) {
					size_t curChar = UITextInput::byteToChar(lines[lineIdx], lineOff);
					++lineIdx;
					size_t pos = 0;
					for (int i = 0; i < lineIdx; ++i)
						pos += lines[i].size() + 1;
					const std::string& nextLine = lines[lineIdx];
					size_t newOff = UITextInput::charToByte(nextLine,
						std::min(curChar, UITextInput::byteToChar(nextLine, nextLine.size())));
					m_cursor = pos + newOff;
					if (!shift) m_selectionAnchor = m_cursor;
					m_isDirty = true;
				}
				return true;
			}
			case SDL_SCANCODE_RETURN: {
				insertNewline();
				return true;
			}
			default:
				return UITextInput::onKeyDown(scancode, shift, ctrl);
		}
	}

	void UITextArea::insertNewline() {
		deleteSelection();
		m_text.insert(m_cursor, "\n");
		m_cursor += 1;
		m_selectionAnchor = m_cursor;
		m_isDirty = true;
	}

	void UITextArea::onRender(const SDL_FRect& screenRect) {
		TTF_Font* font = acquireFont();
		if (!font) return;

		const float padding = 4.0f;
		const float lineSpacing = 2.0f;
		float lineStep = m_fontHeight + lineSpacing;

		auto lines = splitLines(m_text);
		const std::string& displayText = (!m_text.empty() || m_isFocused) ? m_text : m_placeholder;
		bool usePlaceholder = m_text.empty() && !m_isFocused;
		SDL_Color textColor = usePlaceholder ? toSDLColor(m_placeholderColor) : toSDLColor(m_textColor);

		SDL_Rect clipRect{
			static_cast<int>(screenRect.x),
			static_cast<int>(screenRect.y),
			static_cast<int>(screenRect.w),
			static_cast<int>(screenRect.h)
		};
		Renderer::SetClipRect(&clipRect);

		float contentHeight = lines.size() * lineStep;
		float visibleHeight = screenRect.h - padding * 2;
		float maxScroll = std::max(0.0f, contentHeight - visibleHeight);

		// ── 光标移动后自动跟随滚动 ──
		if (!usePlaceholder && m_isFocused) {
			int cursorLine = 0; size_t dummy = 0;
			locateCursor(m_text, m_cursor, lines, cursorLine, dummy);
			float cursorTop  = cursorLine * lineStep - m_scrollY;
			float cursorBot  = cursorTop + m_fontHeight;
			if (cursorTop < 0.0f) {
				m_scrollY = cursorLine * lineStep;
			} else if (cursorBot > visibleHeight) {
				m_scrollY = cursorLine * lineStep + m_fontHeight - visibleHeight;
			}
			m_scrollY = std::clamp(m_scrollY, 0.0f, maxScroll);
		}

		float yPos = screenRect.y + padding - m_scrollY;

		if (usePlaceholder) {
			renderLineTo(font, displayText, textColor, screenRect.x + padding, yPos);
		} else {
			for (size_t i = 0; i < lines.size(); ++i) {
				if (yPos > screenRect.y + screenRect.h) break;

				const std::string& line = lines[i];
				size_t byteStart = 0;
				for (size_t j = 0; j < i; ++j)
					byteStart += lines[j].size() + 1;
				size_t byteEnd = byteStart + line.size();

				// 选区高亮
				if (m_isFocused && m_cursor != m_selectionAnchor) {
					size_t selStart = std::min(m_cursor, m_selectionAnchor);
					size_t selEnd = std::max(m_cursor, m_selectionAnchor);
					if (byteStart < selEnd && byteEnd > selStart) {
						size_t localSelStart = (selStart > byteStart) ? selStart - byteStart : 0;
						size_t localSelEnd = std::min(selEnd - byteStart, line.size());
						std::string beforeSel = line.substr(0, localSelStart);
						std::string selText = line.substr(localSelStart, localSelEnd - localSelStart);
						int beforeW = measureWidth(font, beforeSel.c_str(), beforeSel.length());
						int selW = measureWidth(font, selText.c_str(), selText.length());

						Renderer::SetDrawColor({ 80, 140, 220, 120 });
						Renderer::FillRect(SDL_FRect{
							screenRect.x + padding + static_cast<float>(beforeW),
							yPos,
							std::max(1.0f, static_cast<float>(selW)),
							m_fontHeight
							});
					}
				}

				renderLineTo(font, line, textColor, screenRect.x + padding, yPos);
				yPos += lineStep;
			}

			// ── 光标（在 ClearClipRect 之前绘制，保证 clip 有效） ──
			if (m_isFocused) {
				Uint64 ticks = SDL_GetTicks();
				bool cursorVisible = (ticks / 500) % 2 == 0;

				int lineIdx = 0; size_t lineOff = 0;
				locateCursor(m_text, m_cursor, lines, lineIdx, lineOff);

				float cx = screenRect.x + padding;
				float cy = screenRect.y + padding - m_scrollY + lineIdx * lineStep;
				if (lineOff > 0 && !lines.empty() && lineIdx < static_cast<int>(lines.size())) {
					const std::string& line = lines[lineIdx];
					size_t offsetInLine = std::min(lineOff, line.size());
					cx += static_cast<float>(measureWidth(font, line.c_str(), offsetInLine));
				}

				// IME 光标位置（逻辑坐标 → 窗口物理像素坐标）
				float winX1, winY1, winX2, winY2;
				Renderer::RenderCoordinatesToWindow(cx, cy, &winX1, &winY1);
				Renderer::RenderCoordinatesToWindow(cx + 2.0f, cy + m_fontHeight, &winX2, &winY2);
				SDL_Rect imeRect{
					static_cast<int>(winX1), static_cast<int>(winY1),
					static_cast<int>(winX2 - winX1), static_cast<int>(winY2 - winY1)
				};
				TextInputGate::GetInstance().updateCursorRect(imeRect, 0);

				if (cursorVisible) {
					Renderer::SetDrawColor(m_cursorColor);
					Renderer::FillRect(SDL_FRect{ cx, cy, 2.0f, m_fontHeight });
				}
			}
		}

		// 所有分支都复原 clip，避免泄漏给后续 UI
		Renderer::ClearClipRect();

		m_isDirty = false;
	}
}
