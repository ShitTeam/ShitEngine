#include <SFML/Graphics/Sprite.hpp>
#include "ShitEngine/Component/SpriteRenderer.h"

#include "ShitEngine/Component/TransformComponent.h"
#include "ShitEngine/Resource/ResourceManager.h"
#include "ShitEngine/GameObject/GameObject.h"
#include "ShitEngine/Render/RenderSystem.h"

namespace Shit {
	SpriteRenderer::SpriteRenderer(const std::string& texturePath, const std::optional<sf::IntRect>& rect) : m_sprite(texturePath, rect)
	{
	}

	void SpriteRenderer::onRender() const
	{
		auto* transform = getOwner()->getComponent<TransformComponent>();
		Vector2 position = transform->getPosition();
		Vector2 scale = transform->getScale();
		float angle = transform->getRotation();

		RenderSystem::DrawSprite(m_sprite, position, scale, angle);
	}
}
