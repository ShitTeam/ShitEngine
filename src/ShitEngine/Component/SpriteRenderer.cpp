#include "ShitEngine/Component/SpriteRenderer.h"

#include "ShitEngine/Component/TransformComponent.h"
#include "ShitEngine/Component/CameraComponent.h"
#include "ShitEngine/Resource/ResourceManager.h"
#include "ShitEngine/GameObject/GameObject.h"
#include "ShitEngine/Render/RenderSystem.h"
#include "ShitEngine/Render/Renderer.h"

#include <SDL3/SDL_render.h>
#include <cmath>

namespace Shit {
	void SpriteRenderer::onRender(SDL_Renderer* renderer, const CameraComponent* camera) const
	{
		SDL_Texture* texture = ResourceManager::GetTexture(m_sprite.getTexturePath());
		if (!texture) return;

		auto* transform = getOwner()->getComponent<TransformComponent>();
		if (!transform) return;

		float textureWidth = 0.0f, textureHeight = 0.0f;
		SDL_GetTextureSize(texture, &textureWidth, &textureHeight);

		// 源矩形（用于 sprite-sheet 逐帧）：有则取局部，无则整图
		SDL_FRect srcRect{};
		SDL_FRect* srcPtr = nullptr;
		float frameWidth = textureWidth;
		float frameHeight = textureHeight;
		if (m_sprite.getSourceRect().has_value()) {
			srcRect = m_sprite.getSourceRect().value();
			srcPtr = &srcRect;
			frameWidth = srcRect.w;
			frameHeight = srcRect.h;
		}

		// 世界坐标转屏幕坐标
		Vector2 screenPosition = camera->worldToScreen(transform->getPosition());

		// 像素对齐：取整防止子像素模糊
		float pixelPerUnit = camera->getPixelPerUnit();
		Vector2 scale = transform->getScale();
		SDL_FRect destinationRect = {
			std::floor(screenPosition.x),
			std::floor(screenPosition.y),
			std::floor(frameWidth * scale.x * pixelPerUnit + 0.5f),
			std::floor(frameHeight * scale.y * pixelPerUnit + 0.5f)
		};

		SDL_RenderTextureRotated(renderer, texture, srcPtr, &destinationRect,
			static_cast<double>(transform->getRotation()), nullptr, SDL_FLIP_NONE);
	}

	void SpriteRenderer::setTexturePath(const std::string& texturePath) {
		SDL_Texture* texture = ResourceManager::GetTexture(texturePath);
		if (!texture) {
			ST_CORE_ERROR("无法获取路径为 {} 的纹理！", texturePath);
			return;
		}
		m_sprite.setTexturePath(texturePath);
	}

	SDL_FRect SpriteRenderer::getGlobalBounds() {
		SDL_Texture* texture = ResourceManager::GetTexture(m_sprite.getTexturePath());
		if (!texture) return SDL_FRect{};

		auto* transform = getOwner()->getComponent<TransformComponent>();
		if (!transform) return SDL_FRect{};

		Vector2 position = transform->getPosition();
		Vector2 scale = transform->getScale();

		float width = 0.0f, height = 0.0f;
		if (m_sprite.getSourceRect().has_value()) {
			width = m_sprite.getSourceRect()->w;
			height = m_sprite.getSourceRect()->h;
		} else {
			SDL_GetTextureSize(texture, &width, &height);
		}

		return SDL_FRect{ position.x, position.y, width * scale.x, height * scale.y };
	}
}
