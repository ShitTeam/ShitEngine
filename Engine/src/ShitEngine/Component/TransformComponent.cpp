#include "ShitEngine/Core/pch.h"
#include "ShitEngine/Component/TransformComponent.h"

namespace Shit {
	TransformComponent::TransformComponent() : m_position(0.0f, 0.0f),
		m_scale(1.0f, 1.0f), m_rotation(0.0f) {
	}
}