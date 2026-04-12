#pragma once
#include "../Core/Config.h"
#include "RendererComponent.h"
#include "../Render/Sprite.h"

namespace Shit {
	class GameObject;
	/**
	 * @brief 精灵组件
	 */
	class SHIT_API SpriteRenderer : public RendererComponent {
		friend class GameObject;
	public:
		SpriteRenderer() = default;

		~SpriteRenderer() override = default;

		void onRender(sf::RenderWindow* window) const override; // 重写渲染函数

		// --- setter & getter ---
		void setTexturePath(const std::string& texturePath); // 设置纹理
		//void setOrigin(const sf::Vector2f& origin); // 设置原点

		sf::FloatRect getGlobalBounds() override;
	private:
		std::optional<sf::Sprite> m_sprite = std::nullopt;
	};
}