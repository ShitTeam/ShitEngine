#include "ShitEngine/Event/EventBus.h"

namespace Shit {
	EventBus& EventBus::GetInstance() {
		static EventBus instance;
		return instance;
	}
}
