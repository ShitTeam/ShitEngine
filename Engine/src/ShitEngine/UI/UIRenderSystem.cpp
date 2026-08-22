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
	UIRenderSystem::UIRenderSystem(int priority) : System(priority) {}

	UIRenderSystem::~UIRenderSystem() = default;

	void UIRenderSystem::update() {
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
			if (!owner->isActiveInHierarchy()) continue;   // 失活对象不参与渲染/射线/按钮（一处过滤三处生效）
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
			// 前一个按钮的 onClick 可能已移除本条目对应的渲染器（立即释放组件），
			// 用身份校验跳过已注销的渲染器，避免解引用悬空指针
			if (std::find(m_uiRenderers.begin(), m_uiRenderers.end(), entry.renderer) == m_uiRenderers.end())
				continue;

			GameObject* owner = entry.owner;

			UIButton* button = owner->getComponent<UIButton>();
			if (!button) continue;
			if (std::find(processedButtons.begin(), processedButtons.end(), button) != processedButtons.end())
				continue;
			processedButtons.push_back(button);

			// 复位残留按下：按钮在按下后离开 visible（如 setVisible(false)）导致从未收到 onPointerUp。
			// 鼠标当前未按住且不是本帧释放时视为残留，清理而不触发 onClick。
			if (!mouseUp && !Input::IsMouseButtonPressed(MouseButton::Left) && button->wasPointerDown()) {
				button->resetPressed();
			}

			bool hit = (owner == hoveredGameObject);
			bool isHovered = (button->getState() == UIButton::State::Highlighted
				|| button->getState() == UIButton::State::Pressed);

			if (hit && !isHovered)       button->onPointerEnter();
			else if (!hit && isHovered)   button->onPointerExit();

			if (hit && mouseDown) button->onPointerDown();
			if (mouseUp && (hit || button->getState() == UIButton::State::Pressed || button->wasPointerDown())) button->onPointerUp();
		}

		// --- 输入框聚焦管理（点击时切换） ---
		if (mouseDown) {
			if (hoveredGameObject) {
				UITextInput* textInput = hoveredGameObject->getComponent<UITextInput>();
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
			// onClick 回调可能已立即移除渲染器组件，身份校验后再访问，避免 use-after-free
			if (std::find(m_uiRenderers.begin(), m_uiRenderers.end(), entry.renderer) == m_uiRenderers.end())
				continue;
			entry.renderer->onRender(entry.screenRect);
		}
	}

	void UIRenderSystem::destroy() {
		// 清除输入框焦点，防止悬空指针
		TextInputGate::GetInstance().clearFocus();

		// 重置认领组件的注册状态：系统注销后，已存在组件在同类系统重新注册时
		// 需能被 System::init 补扫重新认领（否则 m_isRegistered 残留 true 会永久失去驱动）
		for (auto* renderer : m_uiRenderers) {
			if (renderer) resetComponent(renderer);
		}
		m_uiRenderers.clear();
	}

	bool UIRenderSystem::onComponentAttached(Component* component) {
		if (auto* renderer = dynamic_cast<UIRendererComponent*>(component)) {
			registerUIRenderer(renderer);
			return true;
		}
		return false;
	}

	void UIRenderSystem::onComponentDetached(Component* component) {
		if (auto* renderer = dynamic_cast<UIRendererComponent*>(component)) {
			unregisterUIRenderer(renderer);
		}
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
