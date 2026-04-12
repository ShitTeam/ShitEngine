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

		// 注册 RendererComponent
		m_owner->getScene()->getSystem<RenderSystem>()->registerRenderer(this);
	}

	void RendererComponent::onDestroy() {
		Component::onDestroy();

		// 注销 RendererComponent
		m_owner->getScene()->getSystem<RenderSystem>()->unregisterRenderer(this);
	}
}
