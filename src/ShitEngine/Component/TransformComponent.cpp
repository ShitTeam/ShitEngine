#include "ShitEngine/Component/TransformComponent.h"

namespace Shit {
	TransformComponent::TransformComponent(Vector2 position, Vector2 scale, float rotation) : m_position(position), m_scale(scale), m_rotation(rotation) {}
}