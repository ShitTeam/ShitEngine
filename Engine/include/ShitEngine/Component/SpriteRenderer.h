#pragma once
#include "../Core/Config.h"
#include "RendererComponent.h"
#include "../Render/Sprite.h"
#include "../Math.h"

#include <optional>
#include <SDL3/SDL_rect.h>

struct SDL_Texture;

namespace Shit {
	class CameraComponent;
	class GameObject;

	/**
	 * @brief 精灵组件
	 *
	 * 内部持有 Sprite 对象描述"画什么"。
	 * 整图渲染时 sourceRect 留空；用于 sprite-sheet 逐帧动画时，
	 * 由 AnimationComponent 把当前帧的 Rect 写回 setSourceRect。
	 */
	class SHIT_API SHIT_REFLECT(BlackList) SpriteRenderer : public RendererComponent {
		friend class GameObject;
		SHIT_REFLECT_BODY(SpriteRenderer)
	public:
		SpriteRenderer() = default;
		~SpriteRenderer() override = default;

		void onRender(SDL_Renderer* renderer, const CameraComponent* camera) const override;

		// --- setter & getter ---
		void setTexturePath(const std::string& texturePath); // 非内联：含纹理存在性检查
		const std::string& getTexturePath() const { return m_sprite.getTexturePath(); }

		void setSourceRect(const std::optional<Rect>& sourceRect) { m_sprite.setSourceRect(sourceRect); }
		const std::optional<Rect>& getSourceRect() const { return m_sprite.getSourceRect(); }

	void setFlipped(bool flipped) { m_sprite.setFlipped(flipped); }

	/// 是否水平翻转
	SHIT_PROPERTY(isFlipped, setFlipped)
	bool isFlipped() const { return m_sprite.isFlipped(); }

	// --- 属性 getter/setter（SHIT_PROPERTY 用）---

	/// Source Rect X
	SHIT_PROPERTY(getSourceRectX, setSourceRectX)
	float getSourceRectX() const;
	void setSourceRectX(float v);

	/// Source Rect Y
	SHIT_PROPERTY(getSourceRectY, setSourceRectY)
	float getSourceRectY() const;
	void setSourceRectY(float v);

	/// Source Rect Width
	SHIT_PROPERTY(getSourceRectW, setSourceRectW)
	float getSourceRectW() const;
	void setSourceRectW(float v);

	/// Source Rect Height
	SHIT_PROPERTY(getSourceRectH, setSourceRectH)
	float getSourceRectH() const;
	void setSourceRectH(float v);

	void onAfterDeserialize() override;  // 反序列化后按 m_texturePath 重载纹理

	SDL_FRect getGlobalBounds() override;
private:
	/// sourceRect 为空（整图渲染）时按当前纹理尺寸构造全图矩形；
	/// 纹理不可用时退化为 {0,0,0,0}
	Rect fullTextureRect() const;

		SHIT_META(({.displayName = "Texture Path", .tooltip = "纹理文件路径（持久化；反序列化后自动重载）"}))
		std::string m_texturePath; ///< 纹理路径（序列化用；m_sprite 因 readOnly 不入档）
		SHIT_META(({.displayName = "Sprite Data", .readOnly = true}))
		Sprite m_sprite; ///< 描述"画什么"的数据
	};
}
