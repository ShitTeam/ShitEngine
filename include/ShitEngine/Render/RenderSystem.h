#pragma once
#include "../Core/Core.h"
#include "../Math.h"
#include "ShitEngine/System/System.h"

struct SDL_Renderer;

namespace Shit {
	class CameraComponent;
	class RendererComponent;

	/**
	 * @brief 渲染系统
	 *
	 * 每帧按优先级排序相机 → 按 Z-Index 排序渲染器 → 逐个相机裁剪渲染。
	 * RendererComponent 的 onAttach / onDetach 自动调用 register / unregister。
	 */
	class SHIT_API RenderSystem final : public System {
	public:
		RenderSystem(int priority = 100);
		~RenderSystem() override;

		void update() override;
		void destroy() override;

		void registerRenderer(RendererComponent *renderer);   ///< 注册渲染组件
		void unregisterRenderer(RendererComponent *renderer); ///< 注销渲染组件
		void registerCamera(CameraComponent *camera);         ///< 注册相机
		void unregisterCamera(CameraComponent *camera);       ///< 注销相机

	private:
		SDL_Renderer* m_renderer = nullptr;

		std::vector<RendererComponent*> m_renderers;
		bool m_isRenderersNeedSort = false;

		std::vector<CameraComponent*> m_cameras;
		bool m_isCamerasNeedSort = false;
	};
}
