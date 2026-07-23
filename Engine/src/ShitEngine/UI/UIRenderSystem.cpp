#include "ShitEngine/Core/pch.h"
#include "ShitEngine/UI/UIRenderSystem.h"

#include "ShitEngine/UI/UIRendererComponent.h"
#include "ShitEngine/UI/UITransform.h"
#include "ShitEngine/UI/UIButton.h"
#include "ShitEngine/UI/UITextInput.h"
#include "ShitEngine/Core/TextInputGate.h"
#include "ShitEngine/GameObject/GameObject.h"
#include "ShitEngine/Input/Input.h"
#include "ShitEngine/Input/KeyCode.h"
#include "ShitEngine/Render/Renderer.h"
#include "ShitEngine/Scene/Scene.h"
#include "ShitEngine/Core/Log.h"

#include <SDL3/SDL_render.h>

namespace Shit {
	UIRenderSystem::UIRenderSystem(int priority) : System(priority) {
		m_renderer = Renderer::GetRenderer();
	}

	UIRenderSystem::~UIRenderSystem() = default;

	void UIRenderSystem::update() {
		if (!m_renderer) return;

		if (m_isRenderersNeedSort) {
			std::stable_sort(m_uiRenderers.begin(), m_uiRenderers.end(),
				[](UIRendererComponent* a, UIRendererComponent* b) {
					return a->getZIndex() < b->getZIndex();
				});
			m_isRenderersNeedSort = false;
		}

		// --- 预计算所有可见控件的 screenRect（raycast 和渲染共用，避免双重 resolveParentRect） ---
		struct CachedRect {
			UIRendererComponent* renderer;
			GameObject* owner;
			SDL_FRect screenRect;
		};
		std::vector<CachedRect> visible;

		for (auto* renderer : m_uiRenderers) {
			if (!renderer || !renderer->isVisible()) continue;
			GameObject* owner = renderer->getOwner();
			if (!owner) continue;
			UITransform* transform = owner->getComponent<UITransform>();
			if (!transform) continue;

			SDL_FRect parentRect = transform->resolveParentRect();
			SDL_FRect screenRect = transform->getScreenRect(&parentRect);
			visible.push_back({renderer, owner, screenRect});
		}

		// --- 输入 Raycasting（从上到下命中，取最上层一个） ---
		Vector2 mousePosition = Input::GetMousePosition();
		GameObject* hoveredGameObject = nullptr;

		for (auto it = visible.rbegin(); it != visible.rend(); ++it) {
			if (it->renderer->containsPoint(it->screenRect, mousePosition)) {
				hoveredGameObject = it->owner;
				break;
			}
		}

		// --- 按钮状态更新（按按钮指针去重，每个按钮一帧只处理一次） ---
		bool mouseDown = Input::IsMouseButtonDown(MouseButton::Left);
		bool mouseUp = Input::IsMouseButtonReleased(MouseButton::Left);

		std::vector<UIButton*> processedButtons;
		for (auto& entry : visible) {
			GameObject* owner = entry.owner;

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
			if (mouseUp && (hit || button->getState() == UIButton::State::Pressed)) button->onPointerUp();
		}

		// --- 输入框聚焦管理（点击时切换） ---
		if (mouseDown) {
			if (hoveredGameObject) {
				UITextInput* textInput = nullptr;
				for (auto& [type, comp] : hoveredGameObject->getComponents()) {
					textInput = dynamic_cast<UITextInput*>(comp.get());
					if (textInput) break;
				}
				if (textInput) {
					TextInputGate::GetInstance().requestFocus(textInput);
				}
			} else {
				TextInputGate::GetInstance().clearFocus();
			}
		}

		// --- 渲染阶段（按 zIndex 从下到上，使用缓存的 screenRect） ---
		Renderer::ClearClipRect();
		Renderer::ClearViewport();

		for (auto& entry : visible) {
			entry.renderer->onRender(entry.screenRect);
		}
	}

	void UIRenderSystem::destroy() {
		// 清除输入框焦点，防止悬空指针
		TextInputGate::GetInstance().clearFocus();

		m_uiRenderers.clear();
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
}
