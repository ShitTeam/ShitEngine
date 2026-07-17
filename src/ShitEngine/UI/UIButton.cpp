#include "ShitEngine/UI/UIButton.h"
#include "ShitEngine/UI/UIImage.h"
#include "ShitEngine/GameObject/GameObject.h"
#include "ShitEngine/Core/Log.h"

namespace Shit {
	void UIButton::setInteractable(bool interactable) {
		if (m_interactable == interactable) return;
		m_interactable = interactable;
		setState(interactable ? State::Normal : State::Disabled);
	}

	void UIButton::onPointerEnter() {
		m_isPointerInside = true;
		if (!m_interactable) return;
		setState(m_isPressed ? State::Pressed : State::Highlighted);
	}

	void UIButton::onPointerExit() {
		m_isPointerInside = false;
		if (!m_interactable) return;
		setState(State::Normal);
	}

	void UIButton::onPointerDown() {
		if (!m_interactable) return;
		m_isPressed = true;
		setState(State::Pressed);
	}

	void UIButton::onPointerUp() {
		if (!m_interactable) return;
		bool wasPressed = m_isPressed;
		m_isPressed = false;

		if (wasPressed && m_isPointerInside) {
			setState(State::Highlighted);
			if (m_onClick) m_onClick();
		} else {
			setState(m_isPointerInside ? State::Highlighted : State::Normal);
		}
	}

	void UIButton::setState(State newState) {
		if (m_state == newState) return;
		m_state = newState;
		applyCurrentColor();
	}

	void UIButton::applyCurrentColor() {
		Color color;
		switch (m_state) {
			case State::Highlighted:  color = m_colors.highlightedColor; break;
			case State::Pressed:       color = m_colors.pressedColor;     break;
			case State::Disabled:      color = m_colors.disabledColor;    break;
			case State::Normal:
			default:                   color = m_colors.normalColor;      break;
		}

		// 把颜色叠加应用到同 GameObject 上的 UIImage（若存在）
		if (m_owner) {
			if (auto* image = m_owner->getComponent<UIImage>()) {
				image->setColor(color);
			}
		}
	}

	void UIButton::onRender(SDL_Renderer* /*renderer*/, const SDL_FRect& /*screenRect*/) {
		// UIButton 自身不绘制，渲染交给同 GameObject 的 UIImage。
		// 状态着色通过 applyCurrentColor 改写 UIImage.color 实现。
	}
}
