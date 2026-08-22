#include "ShitEngine/Core/pch.h"
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

	void RenderSystem::compactRenderers() {
		if (std::find(m_renderers.begin(), m_renderers.end(), nullptr) != m_renderers.end()) {
			m_renderers.erase(
				std::remove(m_renderers.begin(), m_renderers.end(), nullptr),
				m_renderers.end());
		}
	}

	void RenderSystem::compactCameras() {
		if (std::find(m_cameras.begin(), m_cameras.end(), nullptr) != m_cameras.end()) {
			m_cameras.erase(
				std::remove(m_cameras.begin(), m_cameras.end(), nullptr),
				m_cameras.end());
		}
	}

	void RenderSystem::update() {
		// 每帧刷新原生渲染器指针（渲染器重建后不悬垂）
		m_renderer = Renderer::GetRenderer();
		if (!m_renderer) return;

		Renderer::ClearScreen();

		// 排序前压缩墓碑，避免排序比较器解引用空指针
		compactCameras();
		if (m_isCamerasNeedSort) {
			std::sort(m_cameras.begin(), m_cameras.end(), [](CameraComponent* a, CameraComponent* b) {
				return a->getPriority() < b->getPriority();
			});
			m_isCamerasNeedSort = false;
		}

		compactRenderers();
		if (m_isRenderersNeedSort) {
			std::sort(m_renderers.begin(), m_renderers.end(), [](RendererComponent* a, RendererComponent* b) {
				return a->getZIndex() < b->getZIndex();
			});
			m_isRenderersNeedSort = false;
		}

		float logicalW = static_cast<float>(Renderer::GetLogicalWidth());
		float logicalH = static_cast<float>(Renderer::GetLogicalHeight());

		// 按下标遍历（不再每帧全量拷贝）：unregister 只墓碑化（置 null）不 erase，
		// 遍历期间 vector 元素不移动，onRender 中注销组件不会悬垂或跳过。
		for (size_t i = 0; i < m_cameras.size(); ++i) {
			CameraComponent* camera = m_cameras[i];
			if (!camera) continue;
			if (!camera->isEnabled()) continue;   // 禁用的相机不参与本次渲染（编辑器双视图分离用）
			GameObject* camOwner = camera->getOwner();
			if (camOwner && !camOwner->isActiveInHierarchy()) continue;   // 失活对象上的相机不渲染

			SDL_FRect vpRatio = camera->getViewportRatio();
			SDL_Rect viewport;
			viewport.x = static_cast<int>(vpRatio.x * logicalW);
			viewport.y = static_cast<int>(vpRatio.y * logicalH);
			viewport.w = static_cast<int>(vpRatio.w * logicalW);
			viewport.h = static_cast<int>(vpRatio.h * logicalH);

			if (viewport.w <= 0 || viewport.h <= 0) continue;

			// 走 Renderer 抽象（封装视口语义），不再直接操作裸 SDL_Renderer
			Renderer::SetViewport(&viewport);

			// 精准裁剪：Letterbox 纯画面区域（相对于视口原点）
			Vector2 worldSize = camera->getSize();
			if (worldSize.x <= 0 || worldSize.y <= 0) continue;
			float basePpu = (std::min)(static_cast<float>(viewport.w) / worldSize.x, static_cast<float>(viewport.h) / worldSize.y);
			SDL_Rect clipRect;
			clipRect.w = static_cast<int>(worldSize.x * basePpu);
			clipRect.h = static_cast<int>(worldSize.y * basePpu);
			clipRect.x = static_cast<int>((static_cast<float>(viewport.w) - static_cast<float>(clipRect.w)) / 2.0f);
			clipRect.y = static_cast<int>((static_cast<float>(viewport.h) - static_cast<float>(clipRect.h)) / 2.0f);

			Renderer::SetClipRect(&clipRect);

			for (size_t j = 0; j < m_renderers.size(); ++j) {
				RendererComponent* renderer = m_renderers[j];
				if (!renderer || !renderer->isVisible()) continue;
				GameObject* owner = renderer->getOwner();
				if (owner && !owner->isActiveInHierarchy()) continue;   // 失活对象不渲染（随父链级联）
				renderer->onRender(m_renderer, camera);
			}
		}

		// 清除裁剪和视口，恢复全屏（UI 渲染在 UIRenderSystem 中叠加，Present 移至 Game::run 末尾）
		Renderer::ClearClipRect();
		Renderer::ClearViewport();

		// onRender 期间可能新增墓碑，遍历后统一压缩
		compactCameras();
		compactRenderers();
	}

	void RenderSystem::destroy() {
		// 重置认领组件的注册状态：系统注销后，已存在组件在同类系统重新注册时
		// 需能被 System::init 补扫重新认领（否则 m_isRegistered 残留 true 会永久失去驱动）
		for (auto* r : m_renderers) {
			if (r) resetComponent(r);
		}
		for (auto* c : m_cameras) {
			if (c) resetComponent(c);
		}
		m_cameras.clear();
		m_renderers.clear();
		m_renderer = nullptr;
	}

	bool RenderSystem::onComponentAttached(Component* component) {
		if (auto* renderer = dynamic_cast<RendererComponent*>(component)) {
			registerRenderer(renderer);
			return true;
		}
		if (auto* camera = dynamic_cast<CameraComponent*>(component)) {
			registerCamera(camera);
			return true;
		}
		return false;
	}

	void RenderSystem::onComponentDetached(Component* component) {
		if (auto* renderer = dynamic_cast<RendererComponent*>(component)) {
			unregisterRenderer(renderer);
		} else if (auto* camera = dynamic_cast<CameraComponent*>(component)) {
			unregisterCamera(camera);
		}
	}

	void RenderSystem::registerRenderer(RendererComponent* renderer) {
		if (!renderer) {
			auto* scene = getScene();
			ST_CORE_WARN("试图在场景 {} 中注册 RendererComponent 空指针！",
				scene ? scene->getName() : "null");
			return;
		}
		m_renderers.push_back(renderer);
		m_isRenderersNeedSort = true;
	}

	void RenderSystem::unregisterRenderer(RendererComponent* renderer) {
		if (!renderer) {
			auto* scene = getScene();
			ST_CORE_WARN("试图在场景 {} 中移除 RendererComponent 空指针！",
				scene ? scene->getName() : "null");
			return;
		}
		// 墓碑化：update() 遍历结束后统一压缩，避免遍历期间 erase 导致元素前移/悬垂
		for (auto& r : m_renderers) {
			if (r == renderer) {
				r = nullptr;
				break;
			}
		}
	}

	void RenderSystem::registerCamera(CameraComponent* camera) {
		if (!camera) {
			auto* scene = getScene();
			ST_CORE_WARN("试图在场景 {} 中注册 CameraComponent 空指针！",
				scene ? scene->getName() : "null");
			return;
		}
		m_cameras.push_back(camera);
		m_isCamerasNeedSort = true;
	}

	void RenderSystem::unregisterCamera(CameraComponent* camera) {
		if (!camera) {
			auto* scene = getScene();
			ST_CORE_WARN("试图在场景 {} 中移除 CameraComponent 空指针！",
				scene ? scene->getName() : "null");
			return;
		}
		// 墓碑化
		for (auto& c : m_cameras) {
			if (c == camera) {
				c = nullptr;
				break;
			}
		}
	}
}
