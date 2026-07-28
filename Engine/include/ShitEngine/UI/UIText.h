#pragma once
#include "../Core/Core.h"
#include "UIRendererComponent.h"
#include <string>
#include <SDL3_ttf/SDL_ttf.h>

namespace Shit {
	/**
	 * @brief UI 文字控件，使用 SDL_ttf 渲染文字到纹理
	 *
	 * 纹理缓存策略：setText / setFontSize / setColor 置 dirty，
	 * 下次 onRender 用 TTF_RenderText_Blended 重新生成 SDL_Texture。
	 * onDestroy 释放缓存的纹理。字体按 path+size 加载并缓存于进程级映射。
	 * 对齐方式（左 / 中 / 右）相对 UITransform 的屏幕矩形。
	 */
	class SHIT_API SHIT_REFLECT(BlackList) UIText : public UIRendererComponent {
		SHIT_REFLECT_BODY(UIText)
	public:
		enum class TextAnchor { Left, Center, Right };

		UIText() = default;
		UIText(const std::string& text, const std::string& fontPath, float fontSize);
		~UIText() override;

		void onRender(const SDL_FRect& screenRect) override;
		void onDestroy() override;

		// --- getter & setter（变更后置 dirty） ---
		const std::string& getText() const { return m_text; }
		void setText(const std::string& text) { m_text = text; m_isDirty = true; }

		const std::string& getFontPath() const { return m_fontPath; }
		void setFontPath(const std::string& fontPath) { m_fontPath = fontPath; m_isDirty = true; }

		float getFontSize() const { return m_fontSize; }
		void setFontSize(float fontSize) { m_fontSize = fontSize; m_isDirty = true; }

		const Color& getColor() const { return m_color; }
		void setColor(const Color& color) { m_color = color; m_isDirty = true; }

		TextAnchor getAnchor() const { return m_anchor; }
		void setAnchor(TextAnchor anchor) { m_anchor = anchor; }

	private:
		void rebuildTexture();
		TTF_Font* getLoadedFont();

		SHIT_META(({.displayName = "Text", .tooltip = "文字内容"}))
		std::string  m_text;
		SHIT_META(({.displayName = "Font Path", .tooltip = "字体文件路径"}))
		std::string  m_fontPath;
		SHIT_META(({.displayName = "Font Size", .tooltip = "字号", .range = {1, 300}}))
		float        m_fontSize = 24.0f;
		SHIT_META(({.displayName = "Color", .tooltip = "文字颜色"}))
		Color        m_color;
		SHIT_META(({.displayName = "Anchor", .tooltip = "对齐方式（左/中/右）"}))
		TextAnchor   m_anchor = TextAnchor::Center;
		SHIT_META(Disable)
		SDL_Texture* m_cachedTexture = nullptr;
		SHIT_META(Disable)
		bool         m_isDirty = true;
	};
}
