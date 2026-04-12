#include "ShitEngine/Component/Behavior.h"

#include "ShitEngine/GameObject/GameObject.h"
#include "ShitEngine/System/BehaviorSystem.h"

namespace Shit {
	// 因为是DLL库，必须有cpp文件

	void Behavior::onAttach() {
		Component::onAttach();

		// 注册 Behavior

		if (auto* system = m_owner->getScene()->getSystem<BehaviorSystem>()) {
			system->registerBehavior(this);

			m_isRegistered = true;
		}
	}

	void Behavior::onStart() {}
	void Behavior::onUpdate() {}

	void Behavior::onDestroy() {
		Component::onDestroy();

		// 注销 Behavior

		if (auto* system = m_owner->getScene()->getSystem<BehaviorSystem>()) {
			system->unregisterBehavior(this);
		}

		m_isStarted = false;
		m_isRegistered = false;
	}
}
