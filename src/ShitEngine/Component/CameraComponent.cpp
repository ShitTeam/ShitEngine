#include "ShitEngine/Component/CameraComponent.h"
#include "ShitEngine/Core/Window.h"
#include "ShitEngine/Component/TransformComponent.h"
#include "ShitEngine/GameObject/GameObject.h"

namespace Shit {
	CameraComponent::CameraComponent() = default;

	void CameraComponent::onAttach() {
		Component::onAttach();

		// 注册相机
		getOwner()->getScene()->registerCamera(this);
	}

	void CameraComponent::onDestroy() {
		Component::onDestroy();

		// 注销相机
		getOwner()->getScene()->unregisterCamera(this);
	}

	sf::View &CameraComponent::getView()
	{
		// 在获取时同步 Transform 的位置
		auto* transform = getOwner()->getComponent<TransformComponent>();
		Vector2 pos = transform->getPosition();
		
		m_view.setCenter({ pos.x, pos.y });
		m_view.setRotation(sf::degrees(transform->getRotation()));

		return m_view;
	}
}