#include "ShitEngine/Core/pch.h"
#include "ShitEngine/Component/RendererComponent.h"
#include "ShitEngine/GameObject/GameObject.h"
#include "ShitEngine/Render/RenderSystem.h"
#include "ShitEngine/Scene/Scene.h"

namespace Shit {
	RendererComponent::RendererComponent()
		: m_zIndex(0), m_isVisible(true)
	{
	}

	void RendererComponent::onAttach()
	{
		Component::onAttach();

		if (auto* scene = m_owner->getScene()) {
			if (auto* system = scene->getSystem<RenderSystem>()) {
				system->registerRenderer(this);
				m_isRegistered = true;
			}
		}
	}

	void RendererComponent::onDetach() {
		Component::onDetach();

		if (auto* scene = m_owner->getScene()) {
			if (auto* system = scene->getSystem<RenderSystem>()) {
				system->unregisterRenderer(this);
			}
		}

		m_isRegistered = false;
	}

	void RendererComponent::onDestroy() {
		Component::onDestroy();
	}
}
