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
	 */
	class SHIT_API RenderSystem final : public System {
	public:
		RenderSystem(int priority = 100);
		~RenderSystem() override;

		void update() override;
		void destroy() override;

		// 注册渲染组件
		void registerRenderer(RendererComponent *renderer);
		void unregisterRenderer(RendererComponent *renderer);

		// 注册相机
		void registerCamera(CameraComponent *camera);
		void unregisterCamera(CameraComponent *camera);

	private:
		SDL_Renderer* m_renderer = nullptr;

		std::vector<RendererComponent*> m_renderers;
		bool m_isRenderersNeedSort = false;

		std::vector<CameraComponent*> m_cameras;
		bool m_isCamerasNeedSort = false;
	};
}
