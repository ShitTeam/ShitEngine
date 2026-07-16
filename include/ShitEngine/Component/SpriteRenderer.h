#pragma once
#include "../Core/Config.h"
#include "RendererComponent.h"
#include "../Render/Sprite.h"

#include <optional>
#include <SDL3/SDL_rect.h>

struct SDL_Texture;

namespace Shit {
	class CameraComponent;
	class GameObject;

	/**
	 * @brief 精灵组件
	 *
	 * 整图渲染时 m_sourceRect 留空（std::nullopt）；
	 * 用于 sprite-sheet 逐帧动画时，由 AnimationComponent 把当前帧的 SDL_FRect 写回 setSourceRect。
	 */
	class SHIT_API SpriteRenderer : public RendererComponent {
		friend class GameObject;
	public:
		SpriteRenderer() = default;
		~SpriteRenderer() override = default;

		void onRender(SDL_Renderer* renderer, const CameraComponent* camera) const override;

		// --- setter & getter ---
		void setTexturePath(const std::string& texturePath);
		void setSourceRect(const std::optional<SDL_FRect>& sourceRect) { m_sourceRect = sourceRect; }
		const std::optional<SDL_FRect>& getSourceRect() const { return m_sourceRect; }

		SDL_FRect getGlobalBounds() override;
	private:
		std::string m_texturePath; // 纹理资源路径
		std::optional<SDL_FRect> m_sourceRect; // 源矩形（留空 = 整张纹理）
	};
}
