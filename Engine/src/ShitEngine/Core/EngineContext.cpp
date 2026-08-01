#include "ShitEngine/Core/pch.h"
#include "ShitEngine/Core/EngineContext.h"

namespace Shit {
	EngineContext* EngineContext::s_current = nullptr;

	EngineContext& EngineContext::current() {
		if (!s_current) {
			// 进程默认上下文：懒创建，行为与旧单例一致
			static EngineContext defaultContext;
			s_current = &defaultContext;
		}
		return *s_current;
	}

	void EngineContext::setCurrent(EngineContext* ctx) {
		s_current = ctx;
	}

	void EngineContext::resetCurrent() {
		s_current = nullptr;  // 下次 current() 重新懒创建默认上下文
	}
}
