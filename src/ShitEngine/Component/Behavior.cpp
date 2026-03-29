#include "ShitEngine/Component/Behavior.h"

#include "ShitEngine/GameObject/GameObject.h"

namespace Shit {
	// 因为是DLL库，必须有cpp文件

	void Behavior::onAttach() {
		Component::onAttach();

		// 注册 Behavior
		getOwner()->getScene()->registerBehavior(this);
	}

	void Behavior::onStart() {}
	void Behavior::onUpdate() {}

	void Behavior::onDestroy() {
		Component::onDestroy();

		// 注销 Behavior
		getOwner()->getScene()->unregisterBehavior(this);
	}
}
