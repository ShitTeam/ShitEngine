#pragma once
#include "../Core/Core.h"
#include "../Math.h"
#include "ShitEngine/System/System.h"

namespace sf {

}

namespace Shit {
	class CameraComponent;
	class RendererComponent;
	class Sprite;

	/**
	 * @brief 渲染器
	 */
	class SHIT_API RenderSystem final : public System {
	public:
		RenderSystem();
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
		sf::FloatRect getViewBounds(const sf::View& view); // 获取相机在世界坐标系中的矩形

		sf::RenderWindow* m_window = nullptr; // 缓存的窗口

		std::vector<RendererComponent*> m_renderers; // 挂载的渲染组件
		bool m_isRenderersNeedSort = false; // 是否需要排序

		std::vector<CameraComponent*> m_cameras; // 挂载的相机组件
		bool m_isCamerasNeedSort = false; // 是否需要排序
	};
}