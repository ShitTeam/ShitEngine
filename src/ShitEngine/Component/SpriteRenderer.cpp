#include <SFML/Graphics/Sprite.hpp>
#include "ShitEngine/Component/SpriteRenderer.h"

#include "ShitEngine/Component/TransformComponent.h"
#include "ShitEngine/Resource/ResourceManager.h"
#include "ShitEngine/GameObject/GameObject.h"
#include "ShitEngine/Render/RenderSystem.h"

namespace Shit {
	void SpriteRenderer::onRender(sf::RenderWindow* window) const
	{
		window->draw(m_sprite.value());
	}

	void SpriteRenderer::setTexturePath(const std::string &texturePath) {
		auto texture = ResourceManager::GetTexture(texturePath);

		if (!texture) {
			ST_CORE_ERROR("无法获取路径为 {} 的纹理！", texturePath);
			return;
		}

		m_sprite = std::make_optional<sf::Sprite>(*texture);
	}

	sf::FloatRect SpriteRenderer::getGlobalBounds() {
		auto* transform = getOwner()->getComponent<TransformComponent>();
		Vector2 position = transform->getPosition();
		Vector2 scale = transform->getScale();
		float angle = transform->getRotation();

		m_sprite->setPosition({ position.x, position.y });
		m_sprite->setScale({ scale.x, scale.y });
		m_sprite->setRotation(sf::degrees(angle));

		return m_sprite->getGlobalBounds();
	}
}
