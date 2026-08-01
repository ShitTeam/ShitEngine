#include "ShitEngine/Core/pch.h"
#include "ShitEngine/Component/Behavior.h"

#include "ShitEngine/GameObject/GameObject.h"
#include "ShitEngine/System/BehaviorSystem.h"

namespace Shit {

	void Behavior::onCreate() {
		Component::onCreate();
	}

	void Behavior::onAttach() {
		Component::onAttach();

		// 广播给 Scene，由 BehaviorSystem 认领（解耦：不再查询具体系统类型）。
		// 系统未注册时 registerComponent 返回 false → m_isRegistered=false，后续可补挂。
		if (auto* scene = m_owner ? m_owner->getScene() : nullptr) {
			m_isRegistered = scene->registerComponent(this);
		} else {
			m_isRegistered = false;
		}
	}

	void Behavior::onStart() {}
	void Behavior::onUpdate() {}

	void Behavior::onDetach() {
		Component::onDetach();

		if (auto* scene = m_owner ? m_owner->getScene() : nullptr) {
			scene->unregisterComponent(this);
		}

		m_isStarted = false;
		m_isRegistered = false;
	}

	void Behavior::onDestroy() {
		Component::onDestroy();
	}
}
