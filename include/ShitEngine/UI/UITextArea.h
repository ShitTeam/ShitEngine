#pragma once
#include "../Core/Core.h"
#include "UITextInput.h"
#include <string>

namespace Shit {
	/**
	 * @brief 多行文本输入区域
	 *
	 * 支持换行（Enter 插入 '\n'）。自动换行（按宽度软包裹）尚未实现。
	 * 上下方向键按"逻辑行"移动光标（按 '\n' 切分文本）。
	 *
	 * 用法与 UITextBox 一致，构造即可用。
	 */
	class SHIT_API UITextArea : public UITextInput {
	public:
		UITextArea();
		~UITextArea() override = default;

	protected:
		void onRender(const SDL_FRect& screenRect) override;
		bool onKeyDown(SDL_Scancode scancode, bool shift, bool ctrl) override;
		void insertNewline() override;
	};
}
