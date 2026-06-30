#include "ShitEngine/Core/Time.h"

#include "ShitEngine/Core/pch.h"
#include "ShitEngine/Core/Log.h"

namespace Shit {
	Time& Time::GetInstance() {
		static Time instance;
		return instance;
	}

	Time::Time() = default;

	Time::~Time() = default;

	void Time::init() { // 初始化
		m_lastTime = SDL_GetTicksNS();
		m_currentTime = m_lastTime;
		m_totalTime = static_cast<double>(m_currentTime) / 1000000000.0f;

		ST_CORE_TRACE("Time 初始化成功！");
	}

	void Time::update() {
		m_currentTime = SDL_GetTicksNS();

		// 先限制帧率：确保上一帧结束后已过至少 targetFrameTime
		if (m_targetFPS > 0) {
			Uint64 frameElapsed = m_currentTime - m_lastTime;
			Uint64 targetFrameTime = static_cast<Uint64>(1.0e9f / static_cast<float>(m_targetFPS));

			if (frameElapsed < targetFrameTime) {
				SDL_DelayNS(targetFrameTime - frameElapsed);
			}
		}

		// 再计算 deltaTime（包含等待时间）
		m_currentTime = SDL_GetTicksNS();
		m_deltaTime = static_cast<float>(m_currentTime - m_lastTime) / 1000000000.0f;
		m_totalTime = static_cast<double>(m_currentTime) / 1000000000.0f;

		m_lastTime = m_currentTime;
	}
}
