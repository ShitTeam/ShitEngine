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

		// 广播给 Scene，由 RenderSystem 认领（解耦：不再查询具体系统类型）
		if (auto* scene = m_owner ? m_owner->getScene() : nullptr) {
			m_isRegistered = scene->registerComponent(this);
		} else {
			m_isRegistered = false;
		}
	}

	void RendererComponent::onDetach() {
		Component::onDetach();

		if (auto* scene = m_owner ? m_owner->getScene() : nullptr) {
			scene->unregisterComponent(this);
		}

		m_isRegistered = false;
	}

	void RendererComponent::onDestroy() {
		Component::onDestroy();
	}
}
