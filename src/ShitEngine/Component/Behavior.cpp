#include "ShitEngine/Component/Behavior.h"

#include "ShitEngine/GameObject/GameObject.h"
#include "ShitEngine/System/BehaviorSystem.h"

namespace Shit {
	// 因为是DLL库，必须有cpp文件

	void Behavior::onAttach() {
		Component::onAttach();

		// 注册 Behavior
		m_owner->getScene()->getSystem<BehaviorSystem>()->registerBehavior(this);
	}

	void Behavior::onStart() {}
	void Behavior::onUpdate() {}

	void Behavior::onDestroy() {
		Component::onDestroy();

		// 注销 Behavior
		m_owner->getScene()->getSystem<BehaviorSystem>()->unregisterBehavior(this);
	}
}
