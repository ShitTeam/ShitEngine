#include "ShitEngine/Core/pch.h"
#include "ShitEngine/UI/UICanvas.h"
#include "ShitEngine/GameObject/GameObject.h"
#include "ShitEngine/Render/Renderer.h"

namespace Shit {
	void UICanvas::onAttach() {
		Component::onAttach();

		if (auto* scene = m_owner ? m_owner->getScene() : nullptr) {
			m_isRegistered = true;
		}
	}

	void UICanvas::onDetach() {
		Component::onDetach();
		m_isRegistered = false;
	}

	SDL_FRect UICanvas::getScreenRect() const {
		return SDL_FRect{ 0.0f, 0.0f,
			static_cast<float>(Renderer::GetLogicalWidth()),
			static_cast<float>(Renderer::GetLogicalHeight()) };
	}
}
