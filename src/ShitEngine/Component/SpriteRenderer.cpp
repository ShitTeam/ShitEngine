#include "ShitEngine/Component/SpriteRenderer.h"

#include "ShitEngine/Component/TransformComponent.h"
#include "ShitEngine/Resource/ResourceManager.h"
#include "ShitEngine/GameObject/GameObject.h"
#include "ShitEngine/Render/RenderSystem.h"

#include <SDL3/SDL_render.h>

namespace Shit {
	void SpriteRenderer::onRender(SDL_Renderer* renderer) const
	{
		// 获取纹理并检查
		SDL_Texture* texture = ResourceManager::GetTexture(m_texturePath);
		if (!texture) return;

		// 获取 Transform 组件
		auto* transform = getOwner()->getComponent<TransformComponent>();
		if (!transform) return;

		Vector2 position = transform->getPosition();
		Vector2 scale = transform->getScale();

		// 获取纹理原始尺寸
		float textureWidth = 0.0f, textureHeight = 0.0f;
		SDL_GetTextureSize(texture, &textureWidth, &textureHeight);

		// 计算目标矩形（位置 + 缩放）
		SDL_FRect destinationRect = {
			position.x, position.y,
			textureWidth * scale.x,
			textureHeight * scale.y
		};

		// 渲染纹理（支持旋转）
		float rotation = transform->getRotation();
		SDL_RenderTextureRotated(renderer, texture, nullptr, &destinationRect, static_cast<double>(rotation), nullptr, SDL_FLIP_NONE);
	}

	void SpriteRenderer::setTexturePath(const std::string& texturePath) {
		// 提前加载纹理，验证路径有效
		SDL_Texture* texture = ResourceManager::GetTexture(texturePath);
		if (!texture) {
			ST_CORE_ERROR("无法获取路径为 {} 的纹理！", texturePath);
			return;
		}
		m_texturePath = texturePath;
	}

	SDL_FRect SpriteRenderer::getGlobalBounds() {
		// 计算世界坐标中的包围盒，用于视锥体裁剪
		SDL_Texture* texture = ResourceManager::GetTexture(m_texturePath);
		if (!texture) return SDL_FRect{};

		auto* transform = getOwner()->getComponent<TransformComponent>();
		if (!transform) return SDL_FRect{};

		Vector2 position = transform->getPosition();
		Vector2 scale = transform->getScale();

		float textureWidth = 0.0f, textureHeight = 0.0f;
		SDL_GetTextureSize(texture, &textureWidth, &textureHeight);

		return SDL_FRect{ position.x, position.y, textureWidth * scale.x, textureHeight * scale.y };
	}
}
