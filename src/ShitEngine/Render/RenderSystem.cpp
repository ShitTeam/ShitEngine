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

		Renderer::ClearScreen();

		if (m_isCamerasNeedSort) {
			std::sort(m_cameras.begin(), m_cameras.end(), [](CameraComponent* a, CameraComponent* b) {
				return a->getPriority() < b->getPriority();
			});
			m_isCamerasNeedSort = false;
		}

		if (m_isRenderersNeedSort) {
			std::sort(m_renderers.begin(), m_renderers.end(), [](RendererComponent* a, RendererComponent* b) {
				return a->getZIndex() < b->getZIndex();
			});
			m_isRenderersNeedSort = false;
		}

		float logicalW = static_cast<float>(Renderer::GetLogicalWidth());
		float logicalH = static_cast<float>(Renderer::GetLogicalHeight());

		for (auto* camera : m_cameras) {
			SDL_FRect vpRatio = camera->getViewportRatio();

			SDL_Rect viewport;
			viewport.x = static_cast<int>(vpRatio.x * logicalW);
			viewport.y = static_cast<int>(vpRatio.y * logicalH);
			viewport.w = static_cast<int>(vpRatio.w * logicalW);
			viewport.h = static_cast<int>(vpRatio.h * logicalH);

			if (viewport.w <= 0 || viewport.h <= 0) continue;

			// 设置视口（控制渲染原点）
			SDL_SetRenderViewport(m_renderer, &viewport);

			// 精准裁剪：Letterbox 纯画面区域（相对于视口原点）
			Vector2 worldSize = camera->getSize();
			float basePpu = (std::min)(static_cast<float>(viewport.w) / worldSize.x, static_cast<float>(viewport.h) / worldSize.y);
			SDL_Rect clipRect;
			clipRect.x = static_cast<int>((static_cast<float>(viewport.w) - worldSize.x * basePpu) / 2.0f);
			clipRect.y = static_cast<int>((static_cast<float>(viewport.h) - worldSize.y * basePpu) / 2.0f);
			clipRect.w = static_cast<int>(worldSize.x * basePpu);
			clipRect.h = static_cast<int>(worldSize.y * basePpu);

			SDL_SetRenderClipRect(m_renderer, &clipRect);

			for (auto* renderer : m_renderers) {
				if (!renderer->isVisible()) continue;
				renderer->onRender(m_renderer, camera);
			}
		}

		// 清除裁剪和视口，恢复全屏（UI 渲染在 UIRenderSystem 中叠加，Present 移至 Game::run 末尾）
		SDL_SetRenderClipRect(m_renderer, nullptr);
		SDL_SetRenderViewport(m_renderer, nullptr);
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
