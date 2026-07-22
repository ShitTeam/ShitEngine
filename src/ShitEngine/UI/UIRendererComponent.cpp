#include "ShitEngine/Core/pch.h"
#include "ShitEngine/UI/UIRendererComponent.h"
#include "ShitEngine/UI/UIRenderSystem.h"
#include "ShitEngine/GameObject/GameObject.h"
#include "ShitEngine/Scene/Scene.h"
#include "ShitEngine/Math.h"

#include <SDL3/SDL_rect.h>

namespace Shit {
	UIRendererComponent::UIRendererComponent()
		: m_zIndex(0), m_isVisible(true) {}

	void UIRendererComponent::onAttach() {
		Component::onAttach();

		if (auto* scene = m_owner ? m_owner->getScene() : nullptr) {
			if (auto* system = scene->getSystem<UIRenderSystem>()) {
				system->registerUIRenderer(this);
				m_isRegistered = true;
			}
		}
	}

	void UIRendererComponent::onDetach() {
		Component::onDetach();

		if (auto* scene = m_owner ? m_owner->getScene() : nullptr) {
			if (auto* system = scene->getSystem<UIRenderSystem>()) {
				system->unregisterUIRenderer(this);
			}
		}
		m_isRegistered = false;
	}

	void UIRendererComponent::onDestroy() {
		Component::onDestroy();
	}

	bool UIRendererComponent::containsPoint(const SDL_FRect& screenRect, const Vector2& point) const {
		return point.x >= screenRect.x && point.x <= screenRect.x + screenRect.w
			&& point.y >= screenRect.y && point.y <= screenRect.y + screenRect.h;
	}

	void UIRendererComponent::requestSort() {
		if (auto* scene = m_owner ? m_owner->getScene() : nullptr) {
			if (auto* system = scene->getSystem<UIRenderSystem>()) {
				system->markSortDirty();
			}
		}
	}
}
