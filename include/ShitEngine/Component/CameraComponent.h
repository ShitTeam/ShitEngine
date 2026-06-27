#pragma once
#include "../Core/Core.h"
#include "Component.h"
#include "../Math.h"
#include <SDL3/SDL_rect.h>

namespace Shit {
	/**
	 * @brief 相机组件
	 *
	 * 定义"看到的世界范围"（m_worldSize）和在逻辑分辨率上"渲染的区域"（m_viewport）。
	 * m_viewport 是 0~1 的相对值（类似 SFML 的 setViewport），相对于全局逻辑分辨率。
	 * 默认填满整个逻辑窗口。
	 */
	class SHIT_API CameraComponent : public Component {
	public:
		CameraComponent();

		void onAttach() override;
		void onDestroy() override;

		// 坐标转换
		Vector2 worldToScreen(const Vector2& worldPosition) const;
		Vector2 screenToWorld(const Vector2& screenPosition) const;

		// 获取相机中心位置（从 Transform 同步）
		Vector2 getPosition() const;
		Vector2 getSize() const { return m_worldSize; }
		float getZoom() const { return m_zoom; }
		int getPriority() const { return m_priority; }
		float getPixelPerUnit() const;

		void setSize(const Vector2& worldSize) { m_worldSize = worldSize; }
		void setZoom(float zoom) { m_zoom = zoom; }
		void setPriority(int priority) { m_priority = priority; }

		// 比例视口：0~1 相对于逻辑分辨率，默认 {0,0,1,1} = 全屏
		const SDL_FRect& getViewportRatio() const { return m_viewportRatio; }
		void setViewportRatio(const SDL_FRect& ratio) { m_viewportRatio = ratio; }

	private:
		Vector2 m_worldSize{ 1280.0f, 720.0f }; // 看到的世界大小（世界单位）
		float m_zoom = 1.0f;
		int m_priority = 0;

		SDL_FRect m_viewportRatio{ 0.0f, 0.0f, 1.0f, 1.0f }; // 比例视口 (0~1)
	};
}
