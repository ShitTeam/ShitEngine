#include "ShitEngine/Component/CameraComponent.h"
#include "ShitEngine/Core/Window.h"
#include "ShitEngine/Component/TransformComponent.h"
#include "ShitEngine/GameObject/GameObject.h"
#include "ShitEngine/Render/RenderSystem.h"

namespace Shit {
	CameraComponent::CameraComponent() = default;

	void CameraComponent::onAttach() {
		Component::onAttach();

		// 注册相机
		if (auto* system = m_owner->getScene()->getSystem<RenderSystem>()) {
			system->registerCamera(this);
			m_isRegistered = true;
		}
	}

	void CameraComponent::onDestroy() {
		Component::onDestroy();

		// 注销相机
		if (auto* system = m_owner->getScene()->getSystem<RenderSystem>()) {
			system->unregisterCamera(this);
		}

		m_isRegistered = false;
	}
}
