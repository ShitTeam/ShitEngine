#pragma once
#include "UIRendererComponent.h"
#include "../Render/Sprite.h"
#include <string>
#include <optional>

namespace Shit {
	/**
	 * @brief UI 图片控件，在屏幕空间绘制一张精灵
	 *
	 * 继承 UIRendererComponent，由 UIRenderSystem 每帧调用 onRender。
	 * 位置 / 尺寸由同 GameObject 上的 UITransform 决定；Color 用于按钮状态着色。
	 * 不走游戏世界的相机变换，直接用屏幕坐标。
	 */
	class SHIT_API SHIT_REFLECT(BlackList) UIImage : public UIRendererComponent {
		SHIT_REFLECT_BODY(UIImage)
	public:
		UIImage() = default;
		explicit UIImage(const std::string& texturePath);
		~UIImage() override;

		void onRender(const SDL_FRect& screenRect) override;
		void onDestroy() override;

		// --- getter & setter ---
		void setTexturePath(const std::string& texturePath) { m_sprite.setTexturePath(texturePath); }
		const std::string& getTexturePath() const { return m_sprite.getTexturePath(); }

		void setSourceRect(const std::optional<SDL_FRect>& sourceRect) { m_sprite.setSourceRect(sourceRect); }
		const std::optional<SDL_FRect>& getSourceRect() const { return m_sprite.getSourceRect(); }

		void setFlipped(bool flipped) { m_sprite.setFlipped(flipped); }
		bool isFlipped() const { return m_sprite.isFlipped(); }

		const Color& getColor() const { return m_color; }
		void setColor(const Color& color) { m_color = color; }

	private:
		SHIT_META(({.displayName = "Sprite", .readOnly = true}))
		Sprite m_sprite;          ///< 描述"画什么"的数据
		SHIT_META(({.displayName = "Color", .tooltip = "颜色叠加（用于按钮状态切换着色）"}))
		Color  m_color;           ///< 颜色叠加（用于按钮状态切换着色）
	};
}
