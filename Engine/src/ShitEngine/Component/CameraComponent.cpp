#include "ShitEngine/Core/pch.h"
#include "ShitEngine/Component/CameraComponent.h"
#include "ShitEngine/Core/Window.h"
#include "ShitEngine/Component/TransformComponent.h"
#include "ShitEngine/GameObject/GameObject.h"
#include "ShitEngine/Render/RenderSystem.h"
#include "ShitEngine/Render/Renderer.h"

#include <algorithm>
#include <SDL3/SDL_render.h>

namespace Shit {
	CameraComponent::CameraComponent() = default;

	void CameraComponent::onAttach() {
		Component::onAttach();

		if (auto* scene = m_owner->getScene()) {
			if (auto* system = scene->getSystem<RenderSystem>()) {
				system->registerCamera(this);
				m_isRegistered = true;
			} else {
				m_isRegistered = false;  // 系统未注册，允许后续补挂
			}
		}
	}

	void CameraComponent::onDetach() {
		Component::onDetach();

		if (auto* scene = m_owner->getScene()) {
			if (auto* system = scene->getSystem<RenderSystem>()) {
				system->unregisterCamera(this);
			}
		}
		m_isRegistered = false;
	}

	Vector2 CameraComponent::getPosition() const
	{
		if (auto* transform = m_owner->getComponent<TransformComponent>()) {
			return transform->getPosition();
		}
		return Vector2{ 0.0f, 0.0f };
	}

	float CameraComponent::getPixelPerUnit() const
	{
		float logicalW = static_cast<float>(Renderer::GetLogicalWidth());
		float logicalH = static_cast<float>(Renderer::GetLogicalHeight());

		float vpW = m_viewportRatio.w * logicalW;
		float vpH = m_viewportRatio.h * logicalH;

		if (m_worldSize.x <= 0 || m_worldSize.y <= 0 || vpW <= 0 || vpH <= 0) return 1.0f;

		// 基础 PPU 乘以相机自身的缩放系数 zoom
		return (std::min)(vpW / m_worldSize.x, vpH / m_worldSize.y) * m_zoom;
	}

	Vector2 CameraComponent::worldToScreen(const Vector2& worldPosition) const
	{
		// 无效配置（世界尺寸/缩放非正）返回原点，避免除零/NaN 流入渲染
		if (m_worldSize.x <= 0 || m_worldSize.y <= 0 || m_zoom <= 0.0f) return { 0.0f, 0.0f };

		float logicalW = static_cast<float>(Renderer::GetLogicalWidth());
		float logicalH = static_cast<float>(Renderer::GetLogicalHeight());

		float vpW = m_viewportRatio.w * logicalW;
		float vpH = m_viewportRatio.h * logicalH;

		// 1. 不含缩放的黑边适配比例
		float basePpu = (std::min)(vpW / m_worldSize.x, vpH / m_worldSize.y);
		float scaledW = m_worldSize.x * basePpu;
		float scaledH = m_worldSize.y * basePpu;

		// 2. 相对于 Viewport 局部的偏移（与 screenToWorld 同坐标系：视口局部坐标）
		float localOffsetX = (vpW - scaledW) / 2.0f;
		float localOffsetY = (vpH - scaledH) / 2.0f;

		// 3. 带 Zoom 的最终 PPU
		float finalPpu = basePpu * m_zoom;

		// 4. 基于相机中心点进行映射
		Vector2 cameraCenter = getPosition();

		return Vector2{
			localOffsetX + (scaledW * 0.5f) + (worldPosition.x - cameraCenter.x) * finalPpu,
			localOffsetY + (scaledH * 0.5f) + (worldPosition.y - cameraCenter.y) * finalPpu
		};
	}

	Vector2 CameraComponent::screenToWorld(const Vector2& screenPosition) const
	{
		// 无效配置返回相机中心，避免除零/NaN
		if (m_worldSize.x <= 0 || m_worldSize.y <= 0 || m_zoom <= 0.0f) return getPosition();

		float logicalW = static_cast<float>(Renderer::GetLogicalWidth());
		float logicalH = static_cast<float>(Renderer::GetLogicalHeight());

		float vpW = m_viewportRatio.w * logicalW;
		float vpH = m_viewportRatio.h * logicalH;

		float basePpu = (std::min)(vpW / m_worldSize.x, vpH / m_worldSize.y);
		float scaledW = m_worldSize.x * basePpu;
		float scaledH = m_worldSize.y * basePpu;

		// 视口局部坐标（与 worldToScreen 一致，互逆变换）
		float globalOffsetX = (vpW - scaledW) / 2.0f;
		float globalOffsetY = (vpH - scaledH) / 2.0f;

		float finalPpu = basePpu * m_zoom;
		Vector2 cameraCenter = getPosition();

		return Vector2{
			cameraCenter.x + (screenPosition.x - globalOffsetX - (scaledW * 0.5f)) / finalPpu,
			cameraCenter.y + (screenPosition.y - globalOffsetY - (scaledH * 0.5f)) / finalPpu
		};
	}
}
