#pragma once
#include "../Core/Core.h"
#include "../Component/Component.h"
#include <SDL3/SDL_rect.h>

namespace Shit {
	/**
	 * @brief UI 层级根节点
	 *
	 * 挂在有 UITransform 的 GameObject 上，使其成为 UI 层级树的根。
	 * 子物体的 UITransform 若找不到父级 UITransform，会回退到此 Canvas 的屏幕矩形。
	 *
	 * getScreenRect 默认返回渲染器逻辑分辨率矩形（ScreenSpaceOverlay 模式）。
	 * 第一版不实现 Canvas Scaler，缩放依赖 SDL 的 logical presentation letterbox。
	 */
	class SHIT_API SHIT_REFLECT(BlackList) UICanvas : public Component {
		SHIT_REFLECT_BODY(UICanvas)
	public:
		UICanvas() = default;
		~UICanvas() override = default;

		/// @brief Canvas 屏幕矩形（默认 = 渲染器逻辑分辨率）
		SDL_FRect getScreenRect() const;

		// --- getter & setter ---
		int getSortOrder() const { return m_sortOrder; }
		void setSortOrder(int sortOrder) { m_sortOrder = sortOrder; }

	private:
		SHIT_META(({.displayName = "Sort Order", .tooltip = "Canvas 渲染排序"}))
		int m_sortOrder = 0; ///< Canvas 之间的排序（第一版仅记录，多 Canvas 排序后续扩展）
	};
}
