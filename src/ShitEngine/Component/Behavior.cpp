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

		if (auto* scene = m_owner->getScene()) {
			if (auto* system = scene->getSystem<BehaviorSystem>()) {
				system->registerBehavior(this);
				m_isRegistered = true;
			}
		}
	}

	void Behavior::onStart() {}
	void Behavior::onUpdate() {}

	void Behavior::onDetach() {
		Component::onDetach();

		if (auto* scene = m_owner->getScene()) {
			if (auto* system = scene->getSystem<BehaviorSystem>()) {
				system->unregisterBehavior(this);
			}
		}

		m_isStarted = false;
		m_isRegistered = false;
	}

	void Behavior::onDestroy() {
		Component::onDestroy();
	}
}
