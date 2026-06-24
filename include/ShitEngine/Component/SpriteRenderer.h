#pragma once
#include "../Core/Config.h"
#include "RendererComponent.h"
#include "../Render/Sprite.h"

struct SDL_Texture;

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

		void onRender(SDL_Renderer* renderer) const override;

		// --- setter & getter ---
		void setTexturePath(const std::string& texturePath);

		SDL_FRect getGlobalBounds() override;
	private:
		std::string m_texturePath; // 纹理资源路径
	};
}
