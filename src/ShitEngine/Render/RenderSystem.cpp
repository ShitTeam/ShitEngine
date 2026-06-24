#include "ShitEngine/Render/RenderSystem.h"

#include "ShitEngine/Component/CameraComponent.h"
#include "ShitEngine/Core/Window.h"
#include "ShitEngine/Render/Renderer.h"
#include "ShitEngine/Resource/ResourceManager.h"
#include "ShitEngine/Core/Log.h"
#include "ShitEngine/Scene/Scene.h"
#include "ShitEngine/Scene/SceneManager.h"
#include "ShitEngine/Component/RendererComponent.h"
#include "ShitEngine/GameObject/GameObject.h"

#include <SDL3/SDL_render.h>

namespace Shit {
	RenderSystem::RenderSystem(int priority) : System(priority) {
		m_renderer = Renderer::GetRenderer();
	}

	RenderSystem::~RenderSystem() = default;

	void RenderSystem::update() {
		if (!m_renderer) return;

		// 清屏
		SDL_SetRenderDrawColor(m_renderer, 0, 0, 0, 255);
		SDL_RenderClear(m_renderer);

		// 按优先级排序相机（低优先级先渲染）
		if (m_isCamerasNeedSort) {
			std::sort(m_cameras.begin(), m_cameras.end(), [](CameraComponent* a, CameraComponent* b) {
				return a->getPriority() < b->getPriority();
			});
			m_isCamerasNeedSort = false;
		}

		// 按 Z-Index 排序渲染器
		if (m_isRenderersNeedSort) {
			std::sort(m_renderers.begin(), m_renderers.end(), [](RendererComponent* a, RendererComponent* b) {
				return a->getZIndex() < b->getZIndex();
			});
			m_isRenderersNeedSort = false;
		}

		// 逐相机渲染
		for (auto* camera : m_cameras) {
			SDL_SetRenderViewport(m_renderer, nullptr);

			// 获取相机参数
			Vector2 cameraPosition = camera->getPosition();
			Vector2 cameraSize = camera->getSize();
			float cameraZoom = camera->getZoom();

			// 应用相机缩放
			SDL_SetRenderScale(m_renderer, cameraZoom, cameraZoom);

			// 计算相机可见区域（世界坐标）
			SDL_FRect viewBounds = {
				cameraPosition.x - cameraSize.x / 2.0f,
				cameraPosition.y - cameraSize.y / 2.0f,
				cameraSize.x,
				cameraSize.y
			};

			// 渲染可见的物体
			for (auto* renderer : m_renderers) {
				if (!renderer->isVisible()) continue;

				// 视锥体裁剪
				SDL_FRect bounds = renderer->getGlobalBounds();
				SDL_FRect intersection;
				if (SDL_GetRectIntersectionFloat(&viewBounds, &bounds, &intersection)) {
					renderer->onRender(m_renderer);
				}
			}

			// 重置缩放
			SDL_SetRenderScale(m_renderer, 1.0f, 1.0f);
		}

		SDL_RenderPresent(m_renderer);
	}

	void RenderSystem::destroy() {
		m_cameras.clear();
		m_renderers.clear();
		m_renderer = nullptr;
	}

	void RenderSystem::registerRenderer(RendererComponent* renderer) {
		if (!renderer) {
			ST_CORE_WARN("试图在场景 {} 中注册 RendererComponent 空指针！", getScene()->getName());
			return;
		}

		m_renderers.push_back(renderer);
		m_isRenderersNeedSort = true;
	}

	void RenderSystem::unregisterRenderer(RendererComponent* renderer) {
		if (!renderer) {
			ST_CORE_WARN("试图在场景 {} 中移除 RendererComponent 空指针！", getScene()->getName());
			return;
		}

		m_renderers.erase(
			std::remove(m_renderers.begin(), m_renderers.end(), renderer),
			m_renderers.end()
		);
	}

	void RenderSystem::registerCamera(CameraComponent* camera) {
		if (!camera) {
			ST_CORE_WARN("试图在场景 {} 中注册 CameraComponent 空指针！", getScene()->getName());
			return;
		}

		m_cameras.push_back(camera);
		m_isCamerasNeedSort = true;
	}

	void RenderSystem::unregisterCamera(CameraComponent* camera) {
		if (!camera) {
			ST_CORE_WARN("试图在场景 {} 中移除 CameraComponent 空指针！", getScene()->getName());
			return;
		}

		m_cameras.erase(
			std::remove(m_cameras.begin(), m_cameras.end(), camera),
			m_cameras.end()
		);
	}
}
