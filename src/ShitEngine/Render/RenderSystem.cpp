#include "ShitEngine/Render/RenderSystem.h"

#include "ShitEngine/Component/CameraComponent.h"
#include "ShitEngine/Core/Window.h"
#include "ShitEngine/Resource/ResourceManager.h"
#include "ShitEngine/Render/Sprite.h"
#include "ShitEngine/Core/Log.h"
#include "ShitEngine/Scene/Scene.h"
#include "ShitEngine/Scene/SceneManager.h"
#include "ShitEngine/Component/RendererComponent.h"

namespace Shit {
	RenderSystem::RenderSystem() : System(114514) {
		m_window = Window::GetWindow(); // 获取窗口
	}

	RenderSystem::~RenderSystem() = default;

	void RenderSystem::update() {
		m_window->clear(sf::Color::Black); //清屏，设置背景色为黑色

		// 按照 Priority 从小到大排序
		if (m_isCamerasNeedSort) {
			std::sort(m_cameras.begin(), m_cameras.end(), [](CameraComponent* a, CameraComponent* b) {
				return a->getPriority() < b->getPriority();
			});
			m_isCamerasNeedSort = false;
		}

		// 按照 Z-Index 排序，小的在底部
		if (m_isRenderersNeedSort) {
			std::sort(m_renderers.begin(), m_renderers.end(),[](RendererComponent* a, RendererComponent* b) {
				return a->getZIndex() < b->getZIndex();
			});
			m_isRenderersNeedSort = false;
		}

		for (auto* camera : m_cameras) {
			m_window->setView(camera->getView());

			// 获取相机在世界坐标系中的矩形
			sf::FloatRect viewBounds = getViewBounds(m_window->getView());

			for (auto* renderer : m_renderers) {
				if (viewBounds.findIntersection(renderer->getGlobalBounds()).has_value()) // 是否在相机范围内
					renderer->onRender(m_window);
			}
		}

		m_window->display(); //显示渲染结果
	}

	void RenderSystem::destroy() {
		// 清空列表
		m_cameras.clear();
		m_renderers.clear();
	}

	void RenderSystem::registerRenderer(RendererComponent *renderer) {
		if (!renderer) {
			ST_CORE_WARN("试图在场景 {} 中注册 RendererComponent 空指针！", getScene()->getName());
			return;
		}

		m_renderers.push_back(static_cast<RendererComponent*>(renderer));
		m_isRenderersNeedSort = true; // 有新 RendererComponent 插入，需要排序

	}

	void RenderSystem::unregisterRenderer(RendererComponent *renderer) {
		if (!renderer) {
			ST_CORE_WARN("试图在场景 {} 中移除 RendererComponent 空指针！", getScene()->getName());
			return;
		}

		// 从列表内删除
		m_renderers.erase(
			std::remove(m_renderers.begin(), m_renderers.end(), renderer),
			m_renderers.end()
		);
	}

	void RenderSystem::registerCamera(CameraComponent *camera) {
		if (!camera) {
			ST_CORE_WARN("试图在场景 {} 中注册 CameraComponent 空指针！", getScene()->getName());
			return;
		}

		m_cameras.push_back(camera);
		m_isCamerasNeedSort = true;
	}

	void RenderSystem::unregisterCamera(CameraComponent *camera) {
		if (!camera) {
			ST_CORE_WARN("试图在场景 {} 中移除 CameraComponent 空指针！", getScene()->getName());
			return;
		}

		m_cameras.erase(
			std::remove(m_cameras.begin(), m_cameras.end(), camera),
			m_cameras.end()
			);
	}

	sf::FloatRect RenderSystem::getViewBounds(const sf::View &view) {
		sf::Vector2f center = view.getCenter();
		sf::Vector2f size = view.getSize();
		// 计算左上角坐标
		return {{center.x - size.x / 2.f, center.y - size.y / 2.f}, size};
	}
}
