#pragma once
#include "../Core/Core.h"
#include "../Math.h"
#include "ShitEngine/System/System.h"

#include <vector>

struct SDL_Renderer;

namespace Shit {
	class UIRendererComponent;

	/**
	 * @brief UI 渲染系统
	 *
	 * 每帧执行两件事：
	 *   1. 输入 Raycasting：按 zIndex 从上到下检测鼠标命中的 UI 元素，驱动其上的 UIButton 状态；
	 *   2. UI 渲染：按 zIndex 从下到上调用各 UIRendererComponent 的 onRender，叠在游戏世界之上。
	 *
	 * UIRendererComponent 的 onAttach / onDetach 自动注册 / 注销。
	 * priority 默认 200，晚于 RenderSystem(100)，确保 UI 绘制在游戏世界之后。
	 */
	class SHIT_API UIRenderSystem final : public System {
	public:
		UIRenderSystem(int priority = 200);
		~UIRenderSystem() override;

		void update() override;
		void destroy() override;

	void registerUIRenderer(UIRendererComponent* renderer);   ///< 注册 UI 渲染控件
	void unregisterUIRenderer(UIRendererComponent* renderer); ///< 注销 UI 渲染控件

	void markSortDirty() { m_isRenderersNeedSort = true; }     ///< 标记需要重排（zIndex 变更时调用）

private:
	SDL_Renderer* m_renderer = nullptr;

	std::vector<UIRendererComponent*> m_uiRenderers;
	bool m_isRenderersNeedSort = false;
	};
}
