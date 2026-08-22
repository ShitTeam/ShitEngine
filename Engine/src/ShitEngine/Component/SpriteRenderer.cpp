#include "ShitEngine/Core/pch.h"
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
		Texture* textureAsset = ResourceManager::Load<Texture>(m_sprite.getTexturePath());
		SDL_Texture* texture = textureAsset ? textureAsset->get() : nullptr;
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
			srcRect = m_sprite.getSourceRect()->toSDL();
			srcPtr = &srcRect;
			frameWidth = srcRect.w;
			frameHeight = srcRect.h;
		}

		// 世界坐标转屏幕坐标
		Vector2 screenPosition = camera->worldToScreen(transform->getPosition());

		// 像素对齐：取整防止子像素模糊。
		// 位置 = 精灵中心（与 Box2D 刚体/碰撞体/Gizmo 一致）：worldToScreen 把世界点
		// 映射到该点所在像素，用其作为精灵矩形的左上角会造成 +w/2,+h/2 偏移。
		float pixelPerUnit = camera->getPixelPerUnit();
		Vector2 scale = transform->getScale();
		const float w = frameWidth * scale.x * pixelPerUnit;
		const float h = frameHeight * scale.y * pixelPerUnit;
		SDL_FRect destinationRect = {
			std::floor(screenPosition.x - w * 0.5f),
			std::floor(screenPosition.y - h * 0.5f),
			std::floor(w + 0.5f),
			std::floor(h + 0.5f)
		};

		SDL_FlipMode flip = m_sprite.isFlipped() ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
		SDL_RenderTextureRotated(renderer, texture, srcPtr, &destinationRect,
			static_cast<double>(transform->getRotation()), nullptr, flip);
	}

	void SpriteRenderer::setTexturePath(const std::string& texturePath) {
		// 保留路径本身（即使纹理暂时加载失败也保留，便于序列化往返不丢数据）
		m_texturePath = texturePath;
		Texture* textureAsset = ResourceManager::Load<Texture>(texturePath);
		SDL_Texture* texture = textureAsset ? textureAsset->get() : nullptr;
		if (!texture) {
			ST_CORE_ERROR("无法获取路径为 {} 的纹理！", texturePath);
			return;
		}
		m_sprite.setTexturePath(texturePath);
	}

	void SpriteRenderer::onAfterDeserialize() {
		if (m_texturePath.empty()) return;
		// 反射直写字段后 m_sprite 仍为空，这里按路径重载（Load<Texture> 懒加载）
		setTexturePath(m_texturePath);
	}

	SDL_FRect SpriteRenderer::getGlobalBounds() {
		Texture* textureAsset = ResourceManager::Load<Texture>(m_sprite.getTexturePath());
		SDL_Texture* texture = textureAsset ? textureAsset->get() : nullptr;
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

		// 与 onRender 一致：包围盒以 Transform 位置为中心（顶左 → 居中）
		const float gbw = width * scale.x;
		const float gbh = height * scale.y;
		return SDL_FRect{ position.x - gbw * 0.5f, position.y - gbh * 0.5f, gbw, gbh };
	}

// ═══════════════════════════════════════════════════════════════
// 属性 getter/setter 实现
// ═══════════════════════════════════════════════════════════════

Rect SpriteRenderer::fullTextureRect() const {
	Texture* textureAsset = ResourceManager::Load<Texture>(m_texturePath);
	SDL_Texture* texture = textureAsset ? textureAsset->get() : nullptr;
	if (!texture) return Rect{0.0f, 0.0f, 0.0f, 0.0f};
	float w = 0.0f, h = 0.0f;
	SDL_GetTextureSize(texture, &w, &h);
	return Rect{0.0f, 0.0f, w, h};
}

float SpriteRenderer::getSourceRectX() const {
	auto sr = m_sprite.getSourceRect();
	return sr ? sr->x : 0.0f;
}

void SpriteRenderer::setSourceRectX(float v) {
	auto sr = m_sprite.getSourceRect();
	if (!sr) sr = fullTextureRect();   // 整图 → 物化为全纹理矩形，防 0×0 裁没了
	sr->x = v;
	m_sprite.setSourceRect(sr);
}

float SpriteRenderer::getSourceRectY() const {
	auto sr = m_sprite.getSourceRect();
	return sr ? sr->y : 0.0f;
}

void SpriteRenderer::setSourceRectY(float v) {
	auto sr = m_sprite.getSourceRect();
	if (!sr) sr = fullTextureRect();
	sr->y = v;
	m_sprite.setSourceRect(sr);
}

// W/H getter：整图渲染时返回纹理实际尺寸（检查器显示真实值，
// 且序列化物化后语义不变——仍是"画整张图"）
float SpriteRenderer::getSourceRectW() const {
	auto sr = m_sprite.getSourceRect();
	return sr ? sr->w : fullTextureRect().w;
}

void SpriteRenderer::setSourceRectW(float v) {
	auto sr = m_sprite.getSourceRect();
	if (!sr) sr = fullTextureRect();
	sr->w = v;
	m_sprite.setSourceRect(sr);
}

float SpriteRenderer::getSourceRectH() const {
	auto sr = m_sprite.getSourceRect();
	return sr ? sr->h : fullTextureRect().h;
}

void SpriteRenderer::setSourceRectH(float v) {
	auto sr = m_sprite.getSourceRect();
	if (!sr) sr = fullTextureRect();
	sr->h = v;
	m_sprite.setSourceRect(sr);
}
}
