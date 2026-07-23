#pragma once
#include "../Core/Core.h"
#include "UIRendererComponent.h"
#include <functional>

namespace Shit {
	/**
	 * @brief UI 按钮控件，基于鼠标 Raycasting 触发交互
	 *
	 * 由 UIRenderSystem 的命中检测驱动状态机：Normal → Highlighted (悬停) → Pressed (按下)。
	 * 释放且仍在按钮内时触发 onClick。不支持交互时进入 Disabled 状态。
	 * 采用 ColorTint 过渡：状态变化时改写同 GameObject 上 UIImage 的 color（若存在）。
	 * UI 渲染交给同 GameObject 的 UIImage / 自定义控件，本控件不直接绘制。
	 */
	class SHIT_API UIButton : public UIRendererComponent {
	public:
		enum class State { Normal, Highlighted, Pressed, Disabled };

		/// @brief ColorTint 过渡的四种状态颜色（乘到 UIImage 像素上）
		struct ColorBlock {
			Color normalColor      = Color{ 255, 255, 255, 255 };
			Color highlightedColor  = Color{ 230, 230, 230, 255 };
			Color pressedColor      = Color{ 180, 180, 180, 255 };
			Color disabledColor     = Color{ 128, 128, 128, 128 };
		};

		UIButton() = default;
		~UIButton() override = default;

		// --- pointer 事件（由 UIRenderSystem raycasting 调用） ---
		void onPointerEnter();
		void onPointerExit();
		void onPointerDown();
		void onPointerUp();

		// --- getter & setter ---
		bool isInteractable() const { return m_interactable; }
		void setInteractable(bool interactable);

		State getState() const { return m_state; }

		const ColorBlock& getColors() const { return m_colors; }
		void setColors(const ColorBlock& colors) { m_colors = colors; applyCurrentColor(); }

		void setOnClick(std::function<void()> callback) { m_onClick = std::move(callback); }

	protected:
		void onRender(const SDL_FRect& /*screenRect*/) override;

	private:
		void setState(State newState);
		void applyCurrentColor();

		State                 m_state = State::Normal;
		bool                  m_interactable = true;
		bool                  m_isPointerInside = false;
		bool                  m_isPressed = false;
		ColorBlock            m_colors;
		std::function<void()> m_onClick;
	};
}
