#pragma once
#include "../Core/Core.h"
#include "UIRendererComponent.h"
#include <SDL3/SDL_rect.h>
#include <SDL3/SDL_scancode.h>
#include <string>
#include <cstdint>

struct TTF_Font;

namespace Shit {
	/**
	 * @brief 文本输入控件抽象基类
	 *
	 * 负责所有输入控件共享的状态与行为：
	 *   - 文本缓冲（UTF-8）+ 光标位置（以 UTF-8 字符计）
	 *   - 选区（Shift + 方向键 / 鼠标拖拽）
	 *   - 占位符文字（无内容且未聚焦时显示）
	 *   - IME 组合串（preedit）展示与插入
	 *   - 聚焦状态，失焦自动停止文本输入模式
	 *
	 * 单行 vs 多行的差异由子类覆写 onRender / onKeyDown / 插入换行策略实现。
	 * 不直接绘制背景：背景由同 GameObject 上的 UIImage 提供（与按钮一致）。
	 */
	class SHIT_API UITextInput : public UIRendererComponent {
	public:
		UITextInput() = default;
		~UITextInput() override;

		// --- 文本内容 ---
		const std::string& getText() const { return m_text; }
		virtual void setText(const std::string& text);

		// --- 占位符 ---
		const std::string& getPlaceholder() const { return m_placeholder; }
		void setPlaceholder(const std::string& placeholder) { m_placeholder = placeholder; }

		// --- 字体（与 UIText 接口一致，按 path+size 缓存）---
		const std::string& getFontPath() const { return m_fontPath; }
		void setFontPath(const std::string& fontPath) { m_fontPath = fontPath; m_isDirty = true; }

		float getFontSize() const { return m_fontSize; }
		void setFontSize(float fontSize) { m_fontSize = fontSize; m_isDirty = true; }

		float getFontHeight() const { return m_fontHeight; }

		const Color& getTextColor() const { return m_textColor; }
		void setTextColor(const Color& color) { m_textColor = color; m_isDirty = true; }

		const Color& getPlaceholderColor() const { return m_placeholderColor; }
		void setPlaceholderColor(const Color& color) { m_placeholderColor = color; }

		const Color& getCursorColor() const { return m_cursorColor; }
		void setCursorColor(const Color& color) { m_cursorColor = color; }

		// --- 状态查询 ---
		bool isFocused() const { return m_isFocused; }
		bool isMultiline() const { return m_isMultiline; }

		// --- 由 TextInputGate 转发的 SDL 文本事件 ---
		void onTextInput(const char* utf8Text);
		void onTextEditing(const char* utf8Text, int start, int length);

		/// @brief 由 UIRenderSystem 在命中时调用，切换聚焦
		void setFocused(bool focused);

	protected:
		// 子类覆写：把当前"已提交文本 + 光标布局"画到给定矩形内
		void onRender(SDL_Renderer* renderer, const SDL_FRect& screenRect) override = 0;

		/// @brief 子类可覆写：处理键盘控制键（方向键、退格、删除、Home/End、回车）
		/// @return true 表示已处理该按键，不继续传播
		virtual bool onKeyDown(SDL_Scancode scancode, bool shift, bool ctrl);

		/// @brief 子类可覆写：向缓冲插入一段 UTF-8 文本
		virtual void insertText(const std::string& utf8);

		/// @brief 子类可覆写：当前选区是否被替换为换行（UITextBox 拒绝）
		virtual void insertNewline();

		/// @brief 以"字符"计算文本长度
		size_t getCharacterCount() const;

		/// @brief 标记脏位（下一帧重建纹理/布局）
		void markDirty() { m_isDirty = true; }

		/// @brief 获取或加载字体（进程级缓存，跨 UITextInput/UITextBox/UITextArea 共享）
		TTF_Font* acquireFont();

		bool m_isFocused = false;
		bool m_isMultiline = false;
		std::string m_text;
		std::string m_placeholder;
		std::string m_fontPath;
		float m_fontSize = 24.0f;
		float m_fontHeight = 0.0f;
		Color m_textColor      { 30, 30, 30, 255 };
		Color m_placeholderColor { 140, 140, 140, 255 };
		Color m_cursorColor { 80, 140, 220, 255 };
		bool m_isDirty = true;

		// 光标 / 选区（单位：UTF-8 字节偏移）
		size_t m_cursor = 0;
		size_t m_selectionAnchor = 0;

		// IME 组合（preedit）
		std::string m_preedit;
		int m_preeditStart = 0;
		int m_preeditLength = 0;

		// SDL 文本事件编辑辅助
		void deleteSelection();
		void moveCursor(int deltaChars, bool selecting);
		void moveCursorToBoundary(bool toStart, bool selecting);
		void clampCursor();

		// 内部 UTF-8 工具（供子类渲染使用）
		static size_t charToByte(const std::string& s, size_t charIdx);
		static size_t byteToChar(const std::string& s, size_t byteOff);

	private:
		friend class TextInputGate;
		void onDestroy() override;
	};
}
