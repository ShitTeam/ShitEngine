#include "ShitEngine/UI/UIRenderSystem.h"

#include "ShitEngine/UI/UIRendererComponent.h"
#include "ShitEngine/UI/UICanvas.h"
#include "ShitEngine/UI/UITransform.h"
#include "ShitEngine/UI/UIButton.h"
#include "ShitEngine/GameObject/GameObject.h"
#include "ShitEngine/Input/Input.h"
#include "ShitEngine/Input/KeyCode.h"
#include "ShitEngine/Render/Renderer.h"
#include "ShitEngine/Scene/Scene.h"
#include "ShitEngine/Core/Log.h"

#include <SDL3/SDL_render.h>
#include <algorithm>

namespace Shit {
	UIRenderSystem::UIRenderSystem(int priority) : System(priority) {
		m_renderer = Renderer::GetRenderer();
	}

	UIRenderSystem::~UIRenderSystem() = default;

	void UIRenderSystem::update() {
		if (!m_renderer) return;

		if (m_isRenderersNeedSort) {
			std::sort(m_uiRenderers.begin(), m_uiRenderers.end(),
				[](UIRendererComponent* a, UIRendererComponent* b) {
					return a->getZIndex() < b->getZIndex();
				});
			m_isRenderersNeedSort = false;
		}

		// --- 输入 Raycasting（从上到下命中，取最上层一个） ---
		Vector2 mousePosition = Input::GetMousePosition();
		GameObject* hoveredGameObject = nullptr;

		for (auto it = m_uiRenderers.rbegin(); it != m_uiRenderers.rend(); ++it) {
			UIRendererComponent* renderer = *it;
			if (!renderer || !renderer->isVisible()) continue;

			GameObject* owner = renderer->getOwner();
			if (!owner) continue;

			UITransform* transform = owner->getComponent<UITransform>();
			if (!transform) continue;

			SDL_FRect screenRect = transform->getScreenRect(nullptr);
			if (renderer->containsPoint(screenRect, mousePosition)) {
				hoveredGameObject = owner;
				break; // 最上层命中即止
			}
		}

		// --- 按钮状态更新（按按钮指针去重，每个按钮一帧只处理一次） ---
		bool mouseDown = Input::IsMouseButtonDown(MouseButton::Left);
		bool mouseUp = Input::IsMouseButtonReleased(MouseButton::Left);

		std::vector<UIButton*> processedButtons;
		for (auto* renderer : m_uiRenderers) {
			if (!renderer) continue;
			GameObject* owner = renderer->getOwner();
			if (!owner) continue;

			UIButton* button = owner->getComponent<UIButton>();
			if (!button) continue;
			if (std::find(processedButtons.begin(), processedButtons.end(), button) != processedButtons.end())
				continue;
			processedButtons.push_back(button);

			bool hit = (owner == hoveredGameObject);
			bool isHovered = (button->getState() == UIButton::State::Highlighted
				|| button->getState() == UIButton::State::Pressed);

			if (hit && !isHovered)       button->onPointerEnter();
			else if (!hit && isHovered)   button->onPointerExit();

			if (hit && mouseDown) button->onPointerDown();
			if (mouseUp)          button->onPointerUp();
		}

		// --- 渲染阶段（按 zIndex 从下到上） ---
		SDL_SetRenderClipRect(m_renderer, nullptr);
		SDL_SetRenderViewport(m_renderer, nullptr);

		for (auto* renderer : m_uiRenderers) {
			if (!renderer || !renderer->isVisible()) continue;
			GameObject* owner = renderer->getOwner();
			if (!owner) continue;
			UITransform* transform = owner->getComponent<UITransform>();
			if (!transform) continue;

			SDL_FRect screenRect = transform->getScreenRect(nullptr);
			renderer->onRender(m_renderer, screenRect);
		}
	}

	void UIRenderSystem::destroy() {
		m_uiRenderers.clear();
		m_canvases.clear();
		m_renderer = nullptr;
	}

	void UIRenderSystem::registerUIRenderer(UIRendererComponent* renderer) {
		if (!renderer) {
			ST_CORE_WARN("试图注册空 UIRendererComponent！");
			return;
		}
		m_uiRenderers.push_back(renderer);
		m_isRenderersNeedSort = true;
	}

	void UIRenderSystem::unregisterUIRenderer(UIRendererComponent* renderer) {
		if (!renderer) return;
		m_uiRenderers.erase(
			std::remove(m_uiRenderers.begin(), m_uiRenderers.end(), renderer),
			m_uiRenderers.end());
	}

	void UIRenderSystem::registerCanvas(UICanvas* canvas) {
		if (!canvas) return;
		m_canvases.push_back(canvas);
	}

	void UIRenderSystem::unregisterCanvas(UICanvas* canvas) {
		if (!canvas) return;
		m_canvases.erase(
			std::remove(m_canvases.begin(), m_canvases.end(), canvas),
			m_canvases.end());
	}
}
