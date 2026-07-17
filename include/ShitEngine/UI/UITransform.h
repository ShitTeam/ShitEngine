#pragma once
#include "../Core/Core.h"
#include "../Component/Component.h"
#include "../Math.h"
#include <SDL3/SDL_rect.h>
#include <optional>

namespace Shit {
	class GameObject;
	class UICanvas;

	/**
	 * @brief UI 专用变换组件（参考 Unity RectTransform，但不引入 Canvas Scaler）
	 *
	 * 决定 UI 元素在屏幕上的位置和大小。采用锚点（anchor）+ 轴心（pivot）定位：
	 *   - anchorMin/anchorMax 为相对父级矩形的归一化坐标 (0-1)。
	 *   - 若 anchorMin == anchorMax，则元素尺寸由 width/height 决定；
	 *     若 anchorMin != anchorMax，则元素沿该轴拉伸填充父级对应区间。
	 *   - anchoredPosition 为相对锚点的像素偏移。
	 *   - pivot 为元素自身的归一化轴心 (0-1)，影响旋转 / 缩放中心。
	 *
	 * 父级矩形查找顺序：沿 GameObject 父链查最近的 UITransform；若无则用 Canvas 屏幕矩形。
	 */
	class SHIT_API UITransform : public Component {
	public:
		UITransform() = default;
		~UITransform() override = default;

		/// @brief 便捷构造：居中锚点 + 给定偏移与尺寸
		UITransform(float anchoredPositionX, float anchoredPositionY, float width, float height);

		// --- getter & setter ---
		const Vector2& getAnchorMin() const { return m_anchorMin; }
		void setAnchorMin(const Vector2& anchorMin) { m_anchorMin = anchorMin; }

		const Vector2& getAnchorMax() const { return m_anchorMax; }
		void setAnchorMax(const Vector2& anchorMax) { m_anchorMax = anchorMax; }

		const Vector2& getPivot() const { return m_pivot; }
		void setPivot(const Vector2& pivot) { m_pivot = pivot; }

		const Vector2& getAnchoredPosition() const { return m_anchoredPosition; }
		void setAnchoredPosition(const Vector2& position) { m_anchoredPosition = position; }

		float getWidth() const { return m_width; }
		void setWidth(float width) { m_width = width; }

		float getHeight() const { return m_height; }
		void setHeight(float height) { m_height = height; }

		int getZIndex() const { return m_zIndex; }
		void setZIndex(int zIndex) { m_zIndex = zIndex; }

		/// @brief 计算最终屏幕空间矩形（传入已知父级矩形；nullptr 表示用 Canvas 屏幕矩形）
		SDL_FRect getScreenRect(const SDL_FRect* parentRect) const;

		/// @brief 自动查找父级矩形：沿 GameObject 父链找 UITransform，否则用 Canvas；二者都无返回全屏
		SDL_FRect resolveParentRect() const;

	private:
		Vector2 m_anchorMin{ 0.5f, 0.5f };  ///< 锚点左下（归一化 0-1，相对父级）
		Vector2 m_anchorMax{ 0.5f, 0.5f };  ///< 锚点右上
		Vector2 m_pivot{ 0.5f, 0.5f };      ///< 轴心（归一化 0-1，相对自身）
		Vector2 m_anchoredPosition{ 0.0f, 0.0f }; ///< 相对锚点的偏移（像素）
		float   m_width  = 100.0f;           ///< 宽度（仅 anchorMin==anchorMax 轴生效）
		float   m_height = 100.0f;           ///< 高度
		int     m_zIndex = 0;                ///< 渲染层级（值越大越靠上）
	};
}
