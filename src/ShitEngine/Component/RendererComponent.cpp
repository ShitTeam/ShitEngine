#include "ShitEngine/Component/RendererComponent.h"
#include "ShitEngine/GameObject/GameObject.h"
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
		getOwner()->getScene()->registerRenderer(this);
	}

	void RendererComponent::onDestroy() {
		Component::onDestroy();

		// 注销 RendererComponent
		getOwner()->getScene()->unregisterRenderer(this);
	}
}
