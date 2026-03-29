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
		SpriteRenderer(const std::string& texturePath, const std::optional<sf::IntRect>& rect = std::nullopt);
		~SpriteRenderer() override = default;

		void onRender() const override; // 重写渲染函数

		void setTexturePath(const std::string& texturePath) { m_sprite.setTexturePath(texturePath); } // 设置纹理
		//void setOrigin(const sf::Vector2f& origin); // 设置原点
	private:
		Sprite m_sprite; // 精灵
	};
}