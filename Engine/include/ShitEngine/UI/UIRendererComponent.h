#pragma once
#include "../Core/Core.h"
#include "../Component/Component.h"
#include "../Math.h"
#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL_rect.h>

namespace Shit {
	class UIRenderSystem;

	/**
	 * @brief UI 渲染控件基类（对应游戏世界的 RendererComponent）
	 *
	 * 所有可渲染的 UI 控件（UIImage / UIText / UIButton 等）继承此类。
	 * onAttach / onDetach 自动向 UIRenderSystem 注册 / 注销。
	 * 由 UIRenderSystem 每帧按 zIndex 排序后统一调用 onRender，屏幕矩形由 UITransform 计算。
	 */
	class SHIT_API UIRendererComponent : public Component {
	public:
		UIRendererComponent();
		~UIRendererComponent() override = default;

		void onAttach() override;
		void onDetach() override;
		void onDestroy() override;

		/// @brief 子类实现：在给定屏幕矩形内绘制自身（通过 Renderer 静态 API 绘制）
		virtual void onRender(const SDL_FRect& screenRect) = 0;

		/// @brief 点击命中检测：默认矩形内即命中，子类可覆写为更精确的形状
		virtual bool containsPoint(const SDL_FRect& screenRect, const Vector2& point) const;

		// --- getter & setter ---
		int getZIndex() const { return m_zIndex; }
		void setZIndex(int zIndex) { m_zIndex = zIndex; requestSort(); }

		bool isVisible() const { return m_isVisible; }
		void setVisible(bool visible) { m_isVisible = visible; }

	protected:
		/// @brief components变更后通知系统重排（实例：zIndex 变化）
		void requestSort();

		/// @brief Color → SDL_Color 边界转换（供 UIText / UITextInput 系列传给 SDL_ttf 用）
		static SDL_Color toSDLColor(const Color& color) {
			return SDL_Color{ color.red, color.green, color.blue, color.alpha };
		}

		int  m_zIndex = 0;       ///< 渲染层级（值越大越靠上）
		bool m_isVisible = true; ///< 是否参与渲染与命中
	};
}
