#include "ShitEngine/Core/pch.h"
#include "ShitEngine/UI/UIImage.h"
#include "ShitEngine/Render/Renderer.h"
#include "ShitEngine/Resource/ResourceManager.h"
#include "ShitEngine/Core/Log.h"

#include <SDL3/SDL_render.h>

namespace Shit {
	UIImage::UIImage(const std::string& texturePath) {
		m_sprite.setTexturePath(texturePath);
	}

	UIImage::~UIImage() = default;

	void UIImage::onRender(const SDL_FRect& screenRect) {
		SDL_Texture* texture = ResourceManager::GetTexture(m_sprite.getTexturePath());
		if (!texture) {
			ST_CORE_ERROR("UIImage: 无法获取纹理 {}", m_sprite.getTexturePath());
			return;
		}

		// 颜色叠加（按钮状态着色）
		SDL_SetTextureColorMod(texture, m_color.red, m_color.green, m_color.blue);
		SDL_SetTextureAlphaMod(texture, m_color.alpha);

		SDL_FRect srcRect{};
		const SDL_FRect* srcPtr = nullptr;
		if (m_sprite.getSourceRect().has_value()) {
			srcRect = m_sprite.getSourceRect().value();
			srcPtr = &srcRect;
		}

		SDL_FlipMode flip = m_sprite.isFlipped() ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
		Renderer::DrawTextureRotated(texture, srcPtr, screenRect, 0.0, nullptr, flip);

		// 还原默认颜色调制，避免影响后续渲染
		SDL_SetTextureColorMod(texture, 255, 255, 255);
		SDL_SetTextureAlphaMod(texture, 255);
	}

	void UIImage::onDestroy() {
		UIRendererComponent::onDestroy();
	}
}
