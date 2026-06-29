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
	 * 所有的渲染组件必须继承自 RendererComponent ，并重写 onRender 函数
	 */
	class SHIT_API RendererComponent : public Component {
	public:
		RendererComponent();
		~RendererComponent() override = default;

		void onAttach() override;
		virtual void onRender(SDL_Renderer* renderer, const CameraComponent* camera) const = 0;
		void onDestroy() override;

		// --- getter & setter ---
		int getZIndex() const { return m_zIndex; }
		bool isVisible() const { return m_isVisible; }
		virtual SDL_FRect getGlobalBounds() = 0;

		void setZIndex(int zIndex) { m_zIndex = zIndex; }
		void setVisible(bool isVisible) { m_isVisible = isVisible; }

	protected:
		int m_zIndex = 0; // 渲染顺序
		bool m_isVisible = true; // 是否显示
	};
}
