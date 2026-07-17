#include "ShitEngine/UI/UICanvas.h"
#include "ShitEngine/UI/UIRenderSystem.h"
#include "ShitEngine/GameObject/GameObject.h"
#include "ShitEngine/Scene/Scene.h"
#include "ShitEngine/Render/Renderer.h"

namespace Shit {
	void UICanvas::onAttach() {
		Component::onAttach();

		if (auto* scene = m_owner ? m_owner->getScene() : nullptr) {
			if (auto* system = scene->getSystem<UIRenderSystem>()) {
				system->registerCanvas(this);
				m_isRegistered = true;
			}
		}
	}

	void UICanvas::onDetach() {
		Component::onDetach();

		if (auto* scene = m_owner ? m_owner->getScene() : nullptr) {
			if (auto* system = scene->getSystem<UIRenderSystem>()) {
				system->unregisterCanvas(this);
			}
		}
		m_isRegistered = false;
	}

	SDL_FRect UICanvas::getScreenRect() const {
		return SDL_FRect{ 0.0f, 0.0f,
			static_cast<float>(Renderer::GetLogicalWidth()),
			static_cast<float>(Renderer::GetLogicalHeight()) };
	}
}
