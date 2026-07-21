#pragma once
#include "../Core/Core.h"
#include "UITextInput.h"
#include <string>

namespace Shit {
	/**
	 * @brief 单行文本输入框
	 *
	 * 水平单行布局，不接收换行。聚焦时按 Home/End 跳到行首/行尾，左右方向键逐字移动，
	 * Ctrl+Left/Right 跳词（粗略按空格分词）。文字超出可见宽度时自动水平滚动跟随光标。
	 *
	 * 用法：
	 *   auto* go = scene->createGameObject("Input");
	 *   go->setParent(canvas);
	 *   go->addComponent<Shit::UITransform>(...);
	 *   go->addComponent<Shit::UIImage>("input_bg.png");   // 背景可选
	 *   auto* input = go->addComponent<Shit::UITextBox>();
	 *   input->setFontPath("fonts/main.ttf");
	 *   input->setPlaceholder("请输入...");
	 *
	 * 点击控件即聚焦，点其他地方自动失焦（由 UIRenderSystem 驱动）。
	 */
	class SHIT_API UITextBox : public UITextInput {
	public:
		UITextBox();
		~UITextBox() override = default;

		void setCharacterLimit(size_t limit) { m_characterLimit = limit; }
		size_t getCharacterLimit() const { return m_characterLimit; }

	protected:
		void onRender(SDL_Renderer* renderer, const SDL_FRect& screenRect) override;
		void insertText(const std::string& utf8) override;

	private:
		void insertNewline() override; // 单行：拒绝换行
		size_t m_characterLimit = 0;  // 0 = 不限
	};
}
