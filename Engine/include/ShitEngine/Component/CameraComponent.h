#pragma once
#include "../Core/Core.h"
#include "Component.h"
#include "../Math.h"
#include <SDL3/SDL_rect.h>

namespace Shit {
	/**
	 * @brief 相机组件，定义"从哪个角度观察世界"
	 *
	 * 相机的位置由同 GameObject 上的 TransformComponent 决定。
	 * m_worldSize 决定能看到的世界范围，m_viewportRatio 决定渲染到屏幕的哪个区域。
	 * 支持多相机分屏渲染（按 priority 排序）。
	 */
	class SHIT_API CameraComponent : public Component {
	public:
		CameraComponent();

		void onAttach() override;
		void onDetach() override;

		// 坐标转换
		Vector2 worldToScreen(const Vector2& worldPosition) const;  ///< 世界坐标 → 屏幕像素坐标
		Vector2 screenToWorld(const Vector2& screenPosition) const;  ///< 屏幕像素坐标 → 世界坐标

		Vector2 getPosition() const;             ///< 相机中心位置（从 TransformComponent 同步读取）
		Vector2 getSize() const { return m_worldSize; }       ///< 视口世界大小
		float getZoom() const { return m_zoom; }              ///< 缩放系数
		int getPriority() const { return m_priority; }        ///< 渲染优先级（小值先画）
		float getPixelPerUnit() const;                         ///< 每逻辑单位对应的像素数

		void setSize(const Vector2& worldSize) { m_worldSize = worldSize; }
		void setZoom(float zoom) { m_zoom = zoom; }
		void setPriority(int priority) { m_priority = priority; }

		const SDL_FRect& getViewportRatio() const { return m_viewportRatio; }  ///< 视口比例 (0~1)
		void setViewportRatio(const SDL_FRect& ratio) { m_viewportRatio = ratio; } ///< 设置视口比例

	private:
		Vector2 m_worldSize{ 1280.0f, 720.0f };  ///< 看到的世界范围（世界单位）
		float m_zoom = 1.0f;
		int m_priority = 0;
		SDL_FRect m_viewportRatio{ 0.0f, 0.0f, 1.0f, 1.0f }; ///< 相对于逻辑分辨率的裁剪区域 (0~1)
	};
}
