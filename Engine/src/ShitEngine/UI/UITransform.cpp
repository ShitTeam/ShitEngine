#include "ShitEngine/Core/pch.h"
#include "ShitEngine/UI/UITransform.h"

#include "ShitEngine/GameObject/GameObject.h"
#include "ShitEngine/UI/UICanvas.h"
#include "ShitEngine/Render/Renderer.h"

namespace Shit {
	UITransform::UITransform(float anchoredPositionX, float anchoredPositionY, float width, float height)
		: m_anchoredPosition(anchoredPositionX, anchoredPositionY)
		, m_width(width)
		, m_height(height) {}

	SDL_FRect UITransform::getScreenRect(const SDL_FRect* parentRect) const {
		SDL_FRect parent = parentRect ? *parentRect : SDL_FRect{ 0.0f, 0.0f,
			static_cast<float>(Renderer::GetLogicalWidth()),
			static_cast<float>(Renderer::GetLogicalHeight()) };

		// 锚点在父级矩形中的绝对位置
		float anchorMinX = parent.x + m_anchorMin.x * parent.w;
		float anchorMinY = parent.y + m_anchorMin.y * parent.h;
		float anchorMaxX = parent.x + m_anchorMax.x * parent.w;
		float anchorMaxY = parent.y + m_anchorMax.y * parent.h;

		// 元素宽高：拉伸轴用锚点间距，非拉伸轴用 width/height
		float rectWidth  = (m_anchorMax.x != m_anchorMin.x) ? (anchorMaxX - anchorMinX) : m_width;
		float rectHeight = (m_anchorMax.y != m_anchorMin.y) ? (anchorMaxY - anchorMinY) : m_height;

		// 元素左上角 = 锚点中点 + anchoredPosition - pivot * size
		float anchorMidX = (anchorMinX + anchorMaxX) * 0.5f;
		float anchorMidY = (anchorMinY + anchorMaxY) * 0.5f;
		float left = anchorMidX + m_anchoredPosition.x - m_pivot.x * rectWidth;
		float top  = anchorMidY + m_anchoredPosition.y - m_pivot.y * rectHeight;

		return SDL_FRect{ left, top, rectWidth, rectHeight };
	}

	SDL_FRect UITransform::resolveParentRect() const {
		// 沿 GameObject 父链查找最近的 UITransform
		GameObject* parent = m_owner ? m_owner->getParent() : nullptr;
		while (parent) {
			if (auto* parentTransform = parent->getComponent<UITransform>()) {
				// 递归解析父级的父级矩形，确保多层嵌套 UI 布局正确
				SDL_FRect grandParentRect = parentTransform->resolveParentRect();
				return parentTransform->getScreenRect(&grandParentRect);
			}
			parent = parent->getParent();
		}

		// 无父级 UITransform：沿父链找最近的 UICanvas 屏幕矩形
		if (m_owner) {
			GameObject* node = m_owner->getParent();
			while (node) {
				if (auto* canvas = node->getComponent<UICanvas>()) {
					return canvas->getScreenRect();
				}
				node = node->getParent();
			}
		}

		// 都没有：退回逻辑分辨率全屏
		return SDL_FRect{ 0.0f, 0.0f,
			static_cast<float>(Renderer::GetLogicalWidth()),
			static_cast<float>(Renderer::GetLogicalHeight()) };
	}
}
