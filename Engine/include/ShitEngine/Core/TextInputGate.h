#pragma once
#include "../Core/Core.h"

union SDL_Event;
struct SDL_Rect;

namespace Shit {
	class UITextInput;

	/**
	 * @brief 文本输入聚焦门（进程级单例）
	 *
	 * 在任意时刻至多有一个 UITextInput 控件处于"聚焦"状态，负责：
	 *   1. 聚焦时 SDL_StartTextInput(window)，失焦时 SDL_StopTextInput(window)；
	 *   2. 把 SDL 的 TEXT_INPUT / TEXT_EDITING 事件转发给当前聚焦控件（事件指针由
	 *      SDL 持有，控件必须在事件回调里立即拷贝字节）；
	 *   3. 在失焦/销毁时清理 IME 组合（SDL_ClearComposition）。
	 *
	 * UIRenderSystem 通过 setFocus / clearFocus 管理聚焦；UITextInput 析构时自动解绑。
	 */
	class SHIT_API TextInputGate final {
	public:
		TextInputGate(const TextInputGate&) = delete;
		TextInputGate& operator=(const TextInputGate&) = delete;

		static TextInputGate& GetInstance();

		/// @brief 获取原生事件转发入口（由 Input 系统在 PollEvent 后调用）
		static void HandleEvent(const SDL_Event& event) { GetInstance().handleEvent(event); }

		/// @brief 当前是否有聚焦控件
		static bool HasFocus();

		/// @brief 请求聚焦某控件：失焦当前控件，启动新控件的文本输入模式
		void requestFocus(UITextInput* control);

		/// @brief 失焦指定控件（仅当它当前正在聚焦时才生效）
		void releaseFocus(UITextInput* control);

		/// @brief 强制清空焦点（例如场景销毁时）
		void clearFocus();

		/// @brief 通知 IME 光标位置变化（用窗口坐标的 SDL_Rect）
		void updateCursorRect(const SDL_Rect& rect, int cursor);

	private:
		TextInputGate() = default;
		~TextInputGate() = default;

		void handleEvent(const SDL_Event& event);

		UITextInput* m_focused = nullptr;
	};
}
