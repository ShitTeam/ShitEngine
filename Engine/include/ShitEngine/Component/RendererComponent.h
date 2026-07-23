#pragma once
#include "../Core/Core.h"
#include "Component.h"
#include "../Math.h"
#include <SDL3/SDL_rect.h>

struct SDL_Renderer;

namespace Shit {
	class CameraComponent;

	/**
	 * @brief 渲染组件基类
	 *
	 * 所有可渲染的组件（SpriteRenderer 等）须继承此类并重写 onRender。
	 * 由 RenderSystem 每帧按 Z-Index 排序后统一调用。
	 */
	class SHIT_API RendererComponent : public Component {
	public:
		RendererComponent();
		~RendererComponent() override = default;

		void onAttach() override;
		void onDetach() override;
		virtual void onRender(SDL_Renderer* renderer, const CameraComponent* camera) const = 0; ///< 纯虚：子类实现绘制逻辑
		void onDestroy() override;

		// --- getter & setter ---
		int getZIndex() const { return m_zIndex; }           ///< 渲染层级，值越大越靠上
		bool isVisible() const { return m_isVisible; }       ///< 是否可见
		virtual SDL_FRect getGlobalBounds() = 0;             ///< 世界坐标下的轴对齐包围盒

		void setZIndex(int zIndex) { m_zIndex = zIndex; }
		void setVisible(bool isVisible) { m_isVisible = isVisible; }

	protected:
		int m_zIndex = 0;       ///< 渲染顺序（值越大越靠上）
		bool m_isVisible = true; ///< 是否参与渲染
	};
}
